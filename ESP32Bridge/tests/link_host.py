"""ctypes bindings only: all link/session behavior executes production C."""
import ctypes as C
from protocol_host import Frame, Header, View, Workspace, load


class Link(C.Structure):
    _fields_ = [("transport", C.c_int),
                *[(n, C.c_uint32) for n in ("session_id", "next_seq", "rx_high", "rx_reliable_seq", "ack_seq", "tx_seq")],
                ("last_send_ms", C.c_uint64), ("tx_length", C.c_size_t), ("rx_length", C.c_size_t),
                ("retries", C.c_uint),
                *[(n, C.c_bool) for n in ("pending", "sending", "sent", "dead")],
                ("tx", C.c_ubyte * 1024), ("last_rx", C.c_ubyte * 1024)]


class Session(C.Structure):
    _fields_ = [("phase", C.c_int),
                *[(n, C.c_uint32) for n in ("session_id", "previous_session", "tcp_session")],
                *[(n, C.c_bool) for n in ("tcp_open", "tcp_hello", "close_tcp")]]


def library():
    lib = load()
    lib.bridge_test_link_size.restype = C.c_size_t
    lib.bridge_test_session_size.restype = C.c_size_t
    assert lib.bridge_test_link_size() == C.sizeof(Link)
    assert lib.bridge_test_session_size() == C.sizeof(Session)
    signatures = {
        "scb_link_reset": ([C.POINTER(Link), C.c_int, C.c_uint32], None),
        "scb_link_receive": ([C.POINTER(Link), C.c_void_p, C.c_size_t, C.c_bool, C.POINTER(Workspace), C.POINTER(Frame)], C.c_int),
        "scb_link_prepare": ([C.POINTER(Link), C.POINTER(Header), C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(Workspace), C.POINTER(C.c_size_t)], C.c_int),
        "scb_link_sent": ([C.POINTER(Link), C.c_uint32, C.c_uint64], C.c_bool),
        "scb_link_poll": ([C.POINTER(Link), C.c_uint64, C.POINTER(C.c_void_p), C.POINTER(C.c_size_t)], C.c_int),
        "scb_session_init": ([C.POINTER(Session)], None),
        "scb_session_spi": ([C.POINTER(Session), C.c_void_p, C.c_size_t, C.c_uint32, C.POINTER(Workspace), C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)], C.c_int),
        "scb_session_spi_lost": ([C.POINTER(Session)], None),
        "scb_session_tcp_open": ([C.POINTER(Session)], C.c_bool),
        "scb_session_tcp_close": ([C.POINTER(Session)], None),
        "scb_session_tcp_hello": ([C.POINTER(Session), C.c_void_p, C.c_size_t, C.POINTER(Workspace), C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)], C.c_int),
        "scb_session_tcp_permitted": ([C.POINTER(Session), C.c_uint32], C.c_bool),
        "bridge_json_string_equal": ([View, C.c_char_p], C.c_bool),
        "bridge_json_u32": ([View, C.POINTER(C.c_uint32)], C.c_bool),
    }
    for name, (args, result) in signatures.items():
        function = getattr(lib, name)
        function.argtypes, function.restype = args, result
    return lib
