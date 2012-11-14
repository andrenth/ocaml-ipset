#include <string.h>

#include <libipset/session.h>

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/callback.h>
#include <caml/fail.h>
#include <caml/memory.h>

static void
ipset_error(const char *err)
{
    caml_raise_with_string(*caml_named_value("Ipset.Ipset_error"), err);
}

CAMLprim value
caml_ipset_session_init()
{
    CAMLparam0();
    struct ipset_session *session;

    session = ipset_session_init(NULL);
    if (session == NULL)
        ipset_error("cannot initialize session");

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
