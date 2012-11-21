type session

type command
  = Protocol
  | Create
  | Destroy
  | Flush
  | Rename
  | Swap
  | List
  | Save
  | Add
  | Delete
  | Test
  | Header
  | Type
  | Commit

type port_range = int * int

type family
  = Inet
  | Inet6

type bitmap_spec
  = Bitmap_ip of string * int option
  | Bitmap_ip_mac of string
  | Bitmap_port of string

type hash_spec
  = Hash_ip of family * int option * int option * int option
  | Hash_net of family * int option * int option
  | Hash_ip_port of family * int option * int option
  | Hash_net_port of family * int option * int option
  | Hash_ip_port_ip of family * int option * int option
  | Hash_ip_port_net of family * int option * int option

type list_spec
  = List_set of int option

type spec
  = Bitmap of bitmap_spec
  | Hash of hash_spec
  | List of list_spec

exception Ipset_error of string

let () =
  Callback.register_exception "Ipset.Ipset_error" (Ipset_error "")

external load_types : unit -> unit = "caml_ipset_load_types"
external stub_init_session : unit -> session = "caml_ipset_session_init"
external finish_session : session -> unit = "caml_ipset_session_fini"
external create : session -> string -> spec -> float option -> unit
  = "caml_ipset_create"
external destroy : session -> string -> unit = "caml_ipset_destroy"
external flush : session -> string -> unit = "caml_ipset_flush"
external list : session -> string -> unit = "caml_ipset_list"
external save : session -> string -> unit = "caml_ipset_save"
external add : session -> string -> string -> float option -> unit
  = "caml_ipset_add"
external del : session -> string -> string -> unit = "caml_ipset_del"
external test : session -> string -> string -> bool = "caml_ipset_test"
external rename : session -> string -> string -> unit = "caml_ipset_rename"
external swap : session -> string -> string -> unit = "caml_ipset_swap"

let init_session () =
  stub_init_session ()

let init () =
  load_types ();
  stub_init_session ()
