// AlphaSmart 3000 USB/IP device-side adapter, built into the ONE MAME driver.
// This translates host USB URBs into PDIUSBD11 endpoint transactions.
// All descriptor/control/vendor/class responses come from genuine System 3
// through D11 FIFOs and the original interrupt/I2C path (never synthesized).
// Linux-only, opt-in loopback listener: AS3K_USBIP_PORT=3240.
namespace {
u32 usbip_u32(const u8 *p) { return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]); }
void usbip_w32(u8 *p, u32 n)
{
    p[0] = u8(n >> 24); p[1] = u8(n >> 16); p[2] = u8(n >> 8); p[3] = u8(n);
}
void usbip_w16(u8 *p, u16 n) { p[0] = u8(n >> 8); p[1] = u8(n); }
std::array<u8, 312> usbip_device_information()
{
    std::array<u8, 312> d{};
    char const *path = "/sys/devices/platform/as3k-mame/usb1/1-1";
    char const *busid = "1-1";
    std::copy_n(path, strlen(path), d.begin());
    std::copy_n(busid, strlen(busid), d.begin() + 256);
    usbip_w32(&d[288], 1); // bus
    usbip_w32(&d[292], 1); // device
    usbip_w32(&d[296], 2); // full speed USB 1.1
    usbip_w16(&d[300], 0x081e); // original IPD_VENDOR_NUMBER
    usbip_w16(&d[302], 0xbd04); // original IPD_AS3000
    usbip_w16(&d[304], 0x0000);
    d[306] = 0; d[307] = 0; d[308] = 0;
    d[309] = 1; d[310] = 1; d[311] = 1;
    return d;
}
}
void alphasmart3k_state::usbip_queue(const u8 *p, size_t len)
{
    m_usbip_tx.insert(m_usbip_tx.end(), p, p + len);
}
void alphasmart3k_state::usbip_close_peer()
{
    if (m_usbip_peer >= 0) close(m_usbip_peer);
    m_usbip_peer = -1;
    m_usbip_imported = false;
    m_maincpu->irq5_w(0); // physical host power removed
    m_usbip_host_reset_pending = false;
    m_usbip_active = usbip_request{};
    m_usbip_requests.clear();
    m_usbip_rx.clear();
    m_usbip_tx.clear();
    m_usbip_tx_offset = 0;
    m_usbip_active.payload.clear();
    m_usbip_active.received.clear();
}
void alphasmart3k_state::device_stop()
{
#if defined(__linux__)
    if (m_ir_peer >= 0) { ::close(m_ir_peer); m_ir_peer = -1; }
    if (m_ir_server >= 0) { ::close(m_ir_server); m_ir_server = -1; }
#endif

    usbip_close_peer();
    if (m_usbip_server >= 0) close(m_usbip_server);
    m_usbip_server = -1;
}
void alphasmart3k_state::usbip_start()
{
    char const *port_text = std::getenv("AS3K_USBIP_PORT");
    if (!port_text || !*port_text) return;
    int const port = std::atoi(port_text);
    if (port < 1024 || port > 65535) { logerror("AS3K USBIP: invalid port\n"); return; }
    m_usbip_server = socket(AF_INET, SOCK_STREAM, 0);
    if (m_usbip_server < 0) { logerror("AS3K USBIP: socket failed\n"); return; }
    int yes = 1;
    setsockopt(m_usbip_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(u16(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(m_usbip_server, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ||
        listen(m_usbip_server, 1))
    {
        logerror("AS3K USBIP: cannot bind 127.0.0.1:%d\n", port);
        close(m_usbip_server);
        m_usbip_server = -1;
        return;
    }
    fcntl(m_usbip_server, F_SETFL, O_NONBLOCK);
    m_usbip_timer = timer_alloc(FUNC(alphasmart3k_state::usbip_tick), this);
    m_usbip_timer->adjust(attotime::from_msec(1), 0, attotime::from_msec(1));
    logerror("AS3K USBIP: listening on loopback port %d\n", port);
}
void alphasmart3k_state::usbip_reply(int status)
{
    u8 reply[48]{};
    usbip_w32(reply, 3); // USBIP_RET_SUBMIT
    usbip_w32(reply + 4, m_usbip_active.seq);
    usbip_w32(reply + 20, u32(status));
    u32 const actual = status == 0 ? (m_usbip_active.direction ? u32(m_usbip_active.received.size()) : m_usbip_active.payload_offset) : 0;
    usbip_w32(reply + 24, actual);
    usbip_w32(reply + 32, 0xffffffff); // no ISO packet descriptors
    usbip_queue(reply, sizeof(reply));
    if (status == 0 && m_usbip_active.direction && actual)
        usbip_queue(m_usbip_active.received.data(), actual);
    logerror("AS3K USBIP: URB #%u ep=%u direction=%u status=%d length=%u\n",
        m_usbip_active.seq, m_usbip_active.ep, m_usbip_active.direction, status, actual);
    m_usbip_active.phase = 0;
    m_usbip_active.elapsed = 0;
    m_usbip_active.received.clear();
    m_usbip_active.payload.clear();
}
void alphasmart3k_state::usbip_parse()
{
    while (!m_usbip_imported)
    {
        if (m_usbip_rx.size() < 8) return;
        u16 const version = (u16(m_usbip_rx[0]) << 8) | m_usbip_rx[1];
        u16 const operation = (u16(m_usbip_rx[2]) << 8) | m_usbip_rx[3];
        if (version != 0x0111 && version != 0x0106)
        {
            logerror("AS3K USBIP: unsupported protocol %04x\n", version);
            usbip_close_peer(); return;
        }
        if (operation == 0x8005) // OP_REQ_DEVLIST
        {
            m_usbip_rx.erase(m_usbip_rx.begin(), m_usbip_rx.begin() + 8);
            u8 response[12]{};
            usbip_w16(response, version); usbip_w16(response + 2, 0x0005);
            usbip_w32(response + 8, 1);
            usbip_queue(response, sizeof(response));
            auto const info = usbip_device_information();
            usbip_queue(info.data(), info.size());
            u8 const interface_data[4] = {3, 1, 1, 0};
            usbip_queue(interface_data, sizeof(interface_data));
        }
        else if (operation == 0x8003) // OP_REQ_IMPORT + busid[32]
        {
            if (m_usbip_rx.size() < 40) return;
            if (memcmp(m_usbip_rx.data() + 8, "1-1", 3))
            {
                usbip_close_peer(); return;
            }
            m_usbip_rx.erase(m_usbip_rx.begin(), m_usbip_rx.begin() + 40);
            u8 response[8]{};
            usbip_w16(response, version); usbip_w16(response + 2, 0x0003);
            usbip_queue(response, sizeof(response));
            auto const info = usbip_device_information();
            usbip_queue(info.data(), info.size());
            m_usbip_imported = true;
            // Electrical VBUS insertion is an active-high IRQ5 host detection
            // event before the original OS switches PF1 into D11 INT_N polling.
            m_maincpu->irq5_w(1);
            m_usbip_host_reset_pending = true;
            logerror("AS3K USBIP: host imported AS3000 (original firmware required)\n");
        }
        else { usbip_close_peer(); return; }
    }
    while (m_usbip_rx.size() >= 48)
    {
        u32 const command = usbip_u32(m_usbip_rx.data());
        if (command == 2) // USBIP_CMD_UNLINK
        {
            u8 ret[48]{};
            usbip_w32(ret, 4); // USBIP_RET_UNLINK
            usbip_w32(ret + 4, usbip_u32(m_usbip_rx.data() + 4));
            u32 const target = usbip_u32(m_usbip_rx.data() + 20);
            auto const found = std::find_if(m_usbip_requests.begin(), m_usbip_requests.end(),
                [target](usbip_request const &r) { return r.seq == target; });
            bool const cancelled = found != m_usbip_requests.end();
            if (cancelled) m_usbip_requests.erase(found);
            // Successful UNLINK has no later RET_SUBMIT for the cancelled URB.
            usbip_w32(ret + 20, cancelled ? u32(-104) : 0);
            m_usbip_rx.erase(m_usbip_rx.begin(), m_usbip_rx.begin() + 48);
            usbip_queue(ret, sizeof(ret));
            continue;
        }
        if (command != 1) { usbip_close_peer(); return; }
        u32 const dir = usbip_u32(m_usbip_rx.data() + 12);
        u32 const ep = usbip_u32(m_usbip_rx.data() + 16);
        u32 const size = usbip_u32(m_usbip_rx.data() + 24);
        if (dir > 1 || ep > 3 || size > 1024 * 1024) { usbip_close_peer(); return; }
        size_t const packet_bytes = 48 + (dir ? 0 : size);
        if (m_usbip_rx.size() < packet_bytes) return;
        if (m_usbip_requests.size() >= 64) { usbip_close_peer(); return; }
        usbip_request request;
        request.seq = usbip_u32(m_usbip_rx.data() + 4);
        request.devid = usbip_u32(m_usbip_rx.data() + 8);
        request.direction = dir;
        request.ep = ep;
        request.size = size;
        std::copy_n(m_usbip_rx.data() + 40, 8, request.setup.begin());
        request.payload.assign(m_usbip_rx.begin() + 48, m_usbip_rx.begin() + packet_bytes);
        m_usbip_rx.erase(m_usbip_rx.begin(), m_usbip_rx.begin() + packet_bytes);
        request.payload_offset = 0;
        request.received.clear();
        request.elapsed = 0;
        request.phase = (ep == 0) ? 1 : (dir ? 2 : 3);
        logerror("AS3K USBIP: URB #%u SETUP=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X ep=%u len=%u dir=%u\n",
            request.seq, request.setup[0],request.setup[1],request.setup[2],request.setup[3],
            request.setup[4],request.setup[5],request.setup[6],request.setup[7], ep,size,dir);
        m_usbip_requests.push_back(std::move(request));
    }
}
void alphasmart3k_state::usbip_progress()
{
    if (!m_usbip_imported) return;
    if (m_usbip_active.phase && ++m_usbip_active.elapsed > 5000) { usbip_reply(-110); return; }
    if (m_usbip_host_reset_pending)
    {
        // Real USB bus reset occurs only once firmware has enabled SoftConnect.
        if (!m_usb_reset_n || !(m_usb_mode & 0x10)) return;
        usb_bus_reset();
        m_usbip_host_reset_pending = false;
        logerror("AS3K USBIP: USB bus reset delivered to D11 INT_N\n");
    }
    if (!m_usbip_active.phase) return;
    if (!m_usb_reset_n || !(m_usb_mode & 0x10)) return;
    if (m_usbip_active.phase == 1) // Control SETUP token, always EP0 OUT
    {
        if (usb_host_out(2, m_usbip_active.setup.data(), 8, true) < 0) return;
        m_usbip_active.phase = m_usbip_active.direction ? 2 : (m_usbip_active.size ? 3 : 4);
        return;
    }
    unsigned const ep_in = m_usbip_active.ep == 0 ? 3 : (m_usbip_active.ep == 1 ? 4 : m_usbip_active.ep == 2 ? 7 : 9);
    unsigned const ep_out = m_usbip_active.ep == 0 ? 2 : (m_usbip_active.ep == 1 ? 5 : m_usbip_active.ep == 2 ? 6 : 8);
    if (m_usbip_active.phase == 2) // IN packet, original firmware supplies data
    {
        u8 packet[8]{};
        int const len = usb_host_in(ep_in, packet);
        if (len == -2) { usbip_reply(-32); return; } // EPIPE / STALL
        if (len < 0) return;
        unsigned const remaining = m_usbip_active.size > m_usbip_active.received.size() ?
            unsigned(m_usbip_active.size - m_usbip_active.received.size()) : 0;
        m_usbip_active.received.insert(m_usbip_active.received.end(), packet, packet + std::min<unsigned>(len, remaining));
        if (len < 8 || m_usbip_active.received.size() == m_usbip_active.size)
        {
            m_usbip_active.phase = m_usbip_active.ep == 0 ? 5 : 6;
            if (m_usbip_active.phase == 6) usbip_reply(0);
        }
        return;
    }
    if (m_usbip_active.phase == 3) // OUT packet, all data delivered to firmware
    {
        if (m_usbip_active.payload_offset >= m_usbip_active.payload.size())
        {
            if (m_usbip_active.ep && !m_usbip_active.size)
            {
                int const got = usb_host_out(ep_out, nullptr, 0);
                if (got == -2) { usbip_reply(-32); return; }
                if (got < 0) return;
            }
            m_usbip_active.phase = m_usbip_active.ep == 0 ? 4 : 6;
            if (m_usbip_active.phase == 6) usbip_reply(0);
            return;
        }
        unsigned const remain = unsigned(m_usbip_active.payload.size() - m_usbip_active.payload_offset);
        unsigned const len = std::min<unsigned>(8, remain);
        int const got = usb_host_out(ep_out, m_usbip_active.payload.data() + m_usbip_active.payload_offset, len);
        if (got == -2) { usbip_reply(-32); return; }
        if (got < 0) return;
        m_usbip_active.payload_offset += len;
        return;
    }
    if (m_usbip_active.phase == 4) // OUT control transaction: IN ZLP status phase
    {
        u8 dummy[8];
        int const len = usb_host_in(3, dummy);
        if (len == -2) { usbip_reply(-32); return; }
        if (len == 0) usbip_reply(0);
        return;
    }
    if (m_usbip_active.phase == 5) // IN control transaction: OUT ZLP status phase
    {
        if (usb_host_out(2, nullptr, 0) >= 0)
            usbip_reply(0);
    }
}
TIMER_CALLBACK_MEMBER(alphasmart3k_state::usbip_tick)
{
    if (m_usbip_server < 0) return;
    if (m_usbip_peer < 0)
    {
        m_usbip_peer = accept(m_usbip_server, nullptr, nullptr);
        if (m_usbip_peer >= 0)
        {
            fcntl(m_usbip_peer, F_SETFL, O_NONBLOCK);
            m_usbip_imported = false;
            m_usbip_rx.clear();
            m_usbip_tx.clear();
            m_usbip_tx_offset = 0;
        }
    }
    if (m_usbip_peer < 0) return;
    u8 buffer[65536];
    for (;;)
    {
        ssize_t const n = recv(m_usbip_peer, buffer, sizeof(buffer), MSG_DONTWAIT);
        if (n > 0)
        {
            m_usbip_rx.insert(m_usbip_rx.end(), buffer, buffer+n);
            if (m_usbip_rx.size() > 2 * 1024 * 1024) { usbip_close_peer(); return; }
            continue;
        }
        if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR))
        { usbip_close_peer(); return; }
        break;
    }
    usbip_parse();
    if (m_usbip_peer < 0) return;
    if (m_usbip_imported)
    {
        m_usb_frame = (m_usb_frame + 1) & 0x7ff;
        // Independently pending IN URBs must not block OUT or EP0 requests.
        // Preserve ordering on each endpoint/direction and serialize all EP0
        // stages so one SETUP cannot overwrite another control transaction.
        std::array<bool, 8> serviced{};
        size_t const count = m_usbip_requests.size();
        if (!count) usbip_progress();
        for (size_t i = 0; i < count; ++i)
        {
            m_usbip_active = std::move(m_usbip_requests.front());
            m_usbip_requests.pop_front();
            unsigned const key = m_usbip_active.ep ?
                2 * m_usbip_active.ep + m_usbip_active.direction : 0;
            if (!serviced[key])
            {
                serviced[key] = true;
                usbip_progress();
            }
            if (m_usbip_active.phase)
                m_usbip_requests.push_back(std::move(m_usbip_active));
            m_usbip_active = usbip_request{};
        }
    }
    if (m_usbip_tx_offset < m_usbip_tx.size())
    {
        ssize_t const n = send(m_usbip_peer, m_usbip_tx.data() + m_usbip_tx_offset,
            m_usbip_tx.size() - m_usbip_tx_offset, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (n > 0) m_usbip_tx_offset += unsigned(n);
        else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
        { usbip_close_peer(); return; }
    }
    if (m_usbip_tx_offset == m_usbip_tx.size())
    {
        m_usbip_tx.clear();
        m_usbip_tx_offset = 0;
    }
}
