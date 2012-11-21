#include <string.h>

/* XXX this is fixed in newer ipset versions. */
void ipset_port_usage(void) { }

#include <libipset/nfproto.h>
#include <libipset/parse.h>
#include <libipset/session.h>
#include <libipset/types.h>
#include <libipset/ui.h>

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/callback.h>
#include <caml/fail.h>
#include <caml/memory.h>

#define Some_val(v) Field(v, 0)
#define Val_none    Val_int(0)

static void
ipset_error(struct ipset_session *session, char *err)
{
    if (err == NULL) {
        err = (char *)ipset_session_error(session);
        if (err == NULL) {
            err = "unknown error";
        } else {
            size_t last = strlen(err) - 1;
            if (err[last] == '\n')
                err[last] = '\0';
        }
    }
    ipset_session_fini(session);
    caml_raise_with_string(*caml_named_value("Ipset.Ipset_error"), err);
}

CAMLprim value
caml_ipset_load_types(value unit)
{
    CAMLparam1(unit);
    ipset_load_types();
    CAMLreturn(Val_unit);
}

CAMLprim value
caml_ipset_session_init(value unit)
{
    CAMLparam1(unit);
    struct ipset_session *session;

    session = ipset_session_init(printf); /* TODO function as argument */
    if (session == NULL)
        ipset_error(session, "cannot initialize session");

    CAMLreturn((value)session);
}

CAMLprim value
caml_ipset_session_fini(value ml_session)
{
    CAMLparam1(ml_session);
    struct ipset_session *session;

    session = (struct ipset_session *)ml_session;
    ipset_session_fini(session);

    CAMLreturn(Val_unit);
}

static void
check_netmask(struct ipset_session *session, uint8_t netmask)
{
    uint8_t family = ipset_data_family(ipset_session_data(session));

    /* Range limits taken from ipset_parse_netrange(). */
    if (((family == NFPROTO_IPV4) && (netmask < 1 || netmask > 31)) ||
        ((family == NFPROTO_IPV6) && (netmask < 4 || netmask > 124)))
        ipset_error(session, "netmask out of range");
}

static void
parse_bitmap(struct ipset_session *session, value ml_bitmap)
{
    CAMLparam1(ml_bitmap);
    CAMLlocal1(ml_netmask);
    int r;
    const char *typename;
    const struct ipset_type *type;
    char *range;

    ml_netmask = Val_none;

    switch (Tag_val(ml_bitmap)) {
    case 0: /* Bitmap_ip */
        typename = "bitmap:ip";
        range = String_val(Field(ml_bitmap, 0));
        ml_netmask = Field(ml_bitmap, 1);
        break;
    case 1: /* Bitmap_ip_mac */
        typename = "bitmap:ip,mac";
        range = String_val(Field(ml_bitmap, 0));
        break;
    case 2: /* Bitmap_port */
        typename = "bitmap:port";
        range = String_val(Field(ml_bitmap, 0));
        break;
    default:
        ipset_error(session, "invalid bitmap");
    }

    /* Set type */
    r = ipset_parse_typename(session, IPSET_OPT_TYPENAME, typename);
    if (r < 0)
        ipset_error(session, NULL);
    type = ipset_type_get(session, IPSET_CMD_CREATE);
    if (type == NULL)
        ipset_error(session, NULL);

    /* Set arguments */
    r = ipset_parse_netrange(session, IPSET_OPT_IP, range);
    if (r < 0)
        ipset_error(session, NULL);
    if (ml_netmask != Val_none) {
        uint8_t netmask = Int_val(Some_val(ml_netmask));
        check_netmask(session, netmask);
        ipset_session_data_set(session, IPSET_OPT_NETMASK, &netmask);
    }
    CAMLreturn0;
}

static inline int
Family_val(struct ipset_session *session, value ml_family)
{
    CAMLparam1(ml_family);

    switch (Int_val(ml_family)) {
    case 0:
        CAMLreturn(NFPROTO_IPV4);
    case 1:
        CAMLreturn(NFPROTO_IPV6);
    default:
        ipset_error(session, "invalid family");
    }
    /* NOTREACHED */
    CAMLreturn(-1);
}

static void
parse_hash(struct ipset_session *session, value ml_hash)
{
    CAMLparam1(ml_hash);
    CAMLlocal3(ml_hashsize, ml_maxelem, ml_netmask);
    int family;

    family = Family_val(session, Field(ml_hash, 0));
    ipset_session_data_set(session, IPSET_OPT_FAMILY, &family);

    ml_hashsize = Field(ml_hash, 1);
    if (ml_hashsize != Val_none) {
        int hashsize = Int_val(ml_hashsize);
        ipset_session_data_set(session, IPSET_OPT_HASHSIZE, &hashsize);
    }
    ml_maxelem = Field(ml_hash, 2);
    if (ml_maxelem != Val_none) {
        uint32_t maxelem = Int_val(ml_maxelem);
        ipset_session_data_set(session, IPSET_OPT_MAXELEM, &maxelem);
    }

    switch (Tag_val(ml_hash)) {
    case 0: { /* Hash_ip */
        ml_netmask = Field(ml_hash, 3);
        if (ml_netmask != Val_none) {
            uint8_t netmask = Int_val(Some_val(ml_netmask));
            check_netmask(session, netmask);
            ipset_session_data_set(session, IPSET_OPT_NETMASK, &netmask);
        }
    }
    case 1: /* Hash_net */
    case 2: /* Hash_ip_port */
    case 3: /* Hash_net_port */
    case 4: /* Hash_ip_port_ip */
    case 5: /* Hash_ip_port_net */
        /* nothing */
        break;
    default:
        ipset_error(session, "invalid hash");
    }

    CAMLreturn0;
}

static void
parse_list(struct ipset_session *session, value ml_list)
{
    CAMLparam1(ml_list);
    CAMLlocal1(ml_size);

    switch (Tag_val(ml_list)) {
    case 0: {
        ml_size = Field(ml_list, 0);
        if (ml_size != Val_none) {
            int size = Int_val(Some_val(ml_size));
            ipset_session_data_set(session, IPSET_OPT_SIZE, &size);
        }
        break;
    }
    default:
        ipset_error(session, "invalid list");
    }

    CAMLreturn0;
}

static void
parse_type(struct ipset_session *session, value ml_type)
{
    CAMLparam1(ml_type);

    switch (Tag_val(ml_type)) {
    case 0: /* Bitmap */
        parse_bitmap(session, Field(ml_type, 0));
        break;
    case 1: /* Hash */
        parse_hash(session, Field(ml_type, 0));
        break;
    case 2: /* List */
        parse_list(session, Field(ml_type, 0));
        break;
    default:
        ipset_error(session, "invalid set type");
    }

    CAMLreturn0;
}

CAMLprim value
caml_ipset_create(value ml_session, value ml_set, value ml_type, value ml_tmout)
{
    CAMLparam4(ml_session, ml_set, ml_type, ml_tmout);
    int r;
    struct ipset_session *session = (struct ipset_session *)ml_session;
    char *set = String_val(ml_set);

    r = ipset_parse_setname(session, IPSET_SETNAME, set);
    if (r < 0)
        ipset_error(session, NULL);

    parse_type(session, ml_type);

    if (ml_tmout != Val_none) {
        uint32_t timeout = (uint32_t)Double_val(Some_val(ml_tmout));
        ipset_session_data_set(session, IPSET_OPT_TIMEOUT, &timeout);
    }

    r = ipset_cmd(session, IPSET_CMD_CREATE, 0);
    if (r < 0)
        ipset_error(session, NULL);

    CAMLreturn(Val_unit);
}

static void
cmd_with_set(value ml_session, value ml_set, int cmd)
{
    CAMLparam2(ml_session, ml_set);
    struct ipset_session *session = (struct ipset_session *)ml_session;
    char *set = String_val(ml_set);
    int r;

    r = ipset_parse_setname(session, IPSET_SETNAME, set);
    if (r < 0)
        ipset_error(session, NULL);
    r = ipset_cmd(session, cmd, 0);
    if (r < 0)
        ipset_error(session, NULL);

    CAMLreturn0;
}

CAMLprim value
caml_ipset_destroy(value ml_session, value ml_set)
{
    CAMLparam2(ml_session, ml_set);
    cmd_with_set(ml_session, ml_set, IPSET_CMD_DESTROY);
    CAMLreturn(Val_unit);
}

CAMLprim value
caml_ipset_flush(value ml_session, value ml_set)
{
    CAMLparam2(ml_session, ml_set);
    cmd_with_set(ml_session, ml_set, IPSET_CMD_FLUSH);
    CAMLreturn(Val_unit);
}

CAMLprim value
caml_ipset_list(value ml_session, value ml_set)
{
    CAMLparam2(ml_session, ml_set);
    cmd_with_set(ml_session, ml_set, IPSET_CMD_LIST);
    CAMLreturn(Val_unit);
}

CAMLprim value
caml_ipset_save(value ml_session, value ml_set)
{
    CAMLparam2(ml_session, ml_set);
    cmd_with_set(ml_session, ml_set, IPSET_CMD_SAVE);
    CAMLreturn(Val_unit);
}

static void
cmd_with_set_and_entry(value ml_session, value ml_set, value ml_entry, int cmd)
{
    CAMLparam3(ml_session, ml_set, ml_entry);
    int r;
    struct ipset_session *session = (struct ipset_session *)ml_session;
    const struct ipset_type *type;
    char *set = String_val(ml_set);
    char *entry = String_val(ml_entry);

    r = ipset_parse_setname(session, IPSET_SETNAME, set);
    if (r < 0)
        ipset_error(session, NULL);

    type = ipset_type_get(session, cmd);
    if (type == NULL)
        ipset_error(session, NULL);

    r = ipset_parse_elem(session, type->last_elem_optional, entry);
    if (r < 0)
        ipset_error(session, NULL);

    CAMLreturn0;
}

CAMLprim value
caml_ipset_add(value ml_session, value ml_set, value ml_entry, value ml_tmout)
{
    CAMLparam4(ml_session, ml_set, ml_entry, ml_tmout);
    int r;
    struct ipset_session *session = (struct ipset_session *)ml_session;

    if (ml_tmout != Val_none) {
        uint32_t timeout = (uint32_t)Double_val(Some_val(ml_tmout));
        ipset_session_data_set(session, IPSET_OPT_TIMEOUT, &timeout);
    }

    cmd_with_set_and_entry(ml_session, ml_set, ml_entry, IPSET_CMD_ADD);
    r = ipset_cmd(session, IPSET_CMD_ADD, 0);
    if (r < 0)
        ipset_error(session, NULL);

    CAMLreturn(Val_unit);
}

CAMLprim value
caml_ipset_del(value ml_session, value ml_set, value ml_entry)
{
    CAMLparam3(ml_session, ml_set, ml_entry);
    int r;
    struct ipset_session *session = (struct ipset_session *)ml_session;

    cmd_with_set_and_entry(ml_session, ml_set, ml_entry, IPSET_CMD_DEL);
    r = ipset_cmd(session, IPSET_CMD_DEL, 0);
    if (r < 0)
        ipset_error(session, NULL);

    CAMLreturn(Val_unit);
}

CAMLprim value
caml_ipset_test(value ml_session, value ml_set, value ml_entry)
{
    CAMLparam3(ml_session, ml_set, ml_entry);
    int r;
    struct ipset_session *session = (struct ipset_session *)ml_session;

    cmd_with_set_and_entry(ml_session, ml_set, ml_entry, IPSET_CMD_TEST);
    r = ipset_cmd(session, IPSET_CMD_TEST, 0);
    CAMLreturn(r == 0 ? Val_true : Val_false);
}

static void
cmd_with_two_sets(value ml_session, value ml_oldname, value ml_newname,
                  int cmd)
{
    CAMLparam3(ml_session, ml_oldname, ml_newname);
    int r;
    struct ipset_session *session = (struct ipset_session *)ml_session;
    char *oldname = String_val(ml_oldname);
    char *newname = String_val(ml_newname);

    r = ipset_parse_setname(session, IPSET_SETNAME, oldname);
    if (r < 0)
        ipset_error(session, NULL);
    r = ipset_parse_setname(session, IPSET_OPT_SETNAME2, newname);
    if (r < 0)
        ipset_error(session, NULL);

    r = ipset_cmd(session, cmd, 0);
    if (r < 0)
        ipset_error(session, NULL);

    CAMLreturn0;
}

CAMLprim value
caml_ipset_rename(value ml_session, value ml_oldname, value ml_newname)
{
    CAMLparam3(ml_session, ml_oldname, ml_newname);
    cmd_with_two_sets(ml_session, ml_oldname, ml_newname, IPSET_CMD_RENAME);
    CAMLreturn(Val_unit);
}

CAMLprim value
caml_ipset_swap(value ml_session, value ml_oldname, value ml_newname)
{
    CAMLparam3(ml_session, ml_oldname, ml_newname);
    cmd_with_two_sets(ml_session, ml_oldname, ml_newname, IPSET_CMD_SWAP);
    CAMLreturn(Val_unit);
}
