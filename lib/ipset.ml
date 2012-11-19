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

type set_bitmap
  = Bitmap_ip of string * int option
  | Bitmap_ip_mac of string
  | Bitmap_port of string

type set_hash
  = Hash_ip of family * int option * int option * int option
  | Hash_net of family * int option * int option
  | Hash_ip_port of family * int option * int option
  | Hash_net_port of family * int option * int option
  | Hash_ip_port_ip of family * int option * int option
  | Hash_ip_port_net of family * int option * int option

type set_list
  = List_set of int option

type set
  = Bitmap of set_bitmap
  | Hash of set_hash
  | List of set_list

exception Ipset_error of string

let _ = Callback.register_exception "Ipset.Ipset_error" (Ipset_error "")

external load_types : unit -> unit = "caml_ipset_load_types"
external init_session : unit -> session = "caml_ipset_session_init"
external finish_session : session -> unit = "caml_ipset_session_fini"
external create : session -> string -> set -> float option -> unit
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
