type session

external init_session : string -> session = "caml_iptables_session_init"
external finish_session : string -> session = "caml_iptables_session_fini"
