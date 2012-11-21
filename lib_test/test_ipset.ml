open Printf

let set = "test_ipset"
let entry = "10.7.5.99"

let () =
  Ipset.load_types ();
  let session = Ipset.init_session () in
  let set_spec = Ipset.Bitmap (Ipset.Bitmap_ip ("10.7.5.0/24", None)) in
  Ipset.create session set set_spec (Some 10.);
  Ipset.list session set;
  printf "===\n%!";
  Ipset.add session set entry (Some 20.);
  Ipset.list session set;
  Ipset.del session set entry;
  Ipset.destroy session set;
  Ipset.finish_session session
