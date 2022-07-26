#include <yed/plugin.h>
#include <yed/syntax.h>

static yed_syntax syn;

#define _CHECK(x, r)                                                      \
do {                                                                      \
    if (x) {                                                              \
        LOG_FN_ENTER();                                                   \
        yed_log("[!] " __FILE__ ":%d regex error for '%s': %s", __LINE__, \
                r,                                                        \
                yed_syntax_get_regex_err(&syn));                          \
        LOG_EXIT();                                                       \
    }                                                                     \
} while (0)

#define SYN()          yed_syntax_start(&syn)
#define ENDSYN()       yed_syntax_end(&syn)
#define APUSH(s)       yed_syntax_attr_push(&syn, s)
#define APOP(s)        yed_syntax_attr_pop(&syn)
#define RANGE(r)       _CHECK(yed_syntax_range_start(&syn, r), r)
#define ONELINE()      yed_syntax_range_one_line(&syn)
#define SKIP(r)        _CHECK(yed_syntax_range_skip(&syn, r), r)
#define ENDRANGE(r)    _CHECK(yed_syntax_range_end(&syn, r), r)
#define REGEX(r)       _CHECK(yed_syntax_regex(&syn, r), r)
#define REGEXSUB(r, g) _CHECK(yed_syntax_regex_sub(&syn, r, g), r)
#define KWD(k)         yed_syntax_kwd(&syn, k)

#ifdef __APPLE__
#define WB "[[:>:]]"
#else
#define WB "\\b"
#endif

void estyle(yed_event *event)   { yed_syntax_style_event(&syn, event);         }
void ebuffdel(yed_event *event) { yed_syntax_buffer_delete_event(&syn, event); }
void ebuffmod(yed_event *event) { yed_syntax_buffer_mod_event(&syn, event);    }
void eline(yed_event *event)  {
    yed_frame *frame;

    frame = event->frame;

    if (!frame
    ||  !frame->buffer
    ||  frame->buffer->kind != BUFF_KIND_FILE
    ||  frame->buffer->ft != yed_get_ft("BPF")) {
        return;
    }

    yed_syntax_line_event(&syn, event);
}


void unload(yed_plugin *self) {
    yed_syntax_free(&syn);
}

int yed_plugin_boot(yed_plugin *self) {
    yed_event_handler style;
    yed_event_handler buffdel;
    yed_event_handler buffmod;
    yed_event_handler line;


    YED_PLUG_VERSION_CHECK();

    yed_plugin_set_unload_fn(self, unload);

    style.kind = EVENT_STYLE_CHANGE;
    style.fn   = estyle;
    yed_plugin_add_event_handler(self, style);

    buffdel.kind = EVENT_BUFFER_PRE_DELETE;
    buffdel.fn   = ebuffdel;
    yed_plugin_add_event_handler(self, buffdel);

    buffmod.kind = EVENT_BUFFER_POST_MOD;
    buffmod.fn   = ebuffmod;
    yed_plugin_add_event_handler(self, buffmod);

    line.kind = EVENT_LINE_PRE_DRAW;
    line.fn   = eline;
    yed_plugin_add_event_handler(self, line);


    SYN();
        APUSH("&code-comment");
            RANGE("/\\*");
            ENDRANGE(  "\\*/");
            RANGE("//");
                ONELINE();
            ENDRANGE("$");
            RANGE("^[[:space:]]*#[[:space:]]*if[[:space:]]+0"WB);
            ENDRANGE("^[[:space:]]*#[[:space:]]*(else|endif|elif|elifdef)"WB);
        APOP();

        APUSH("&code-string");
            RANGE("\""); SKIP("\\\\\""); ENDRANGE("\"");
            RANGE("'");  SKIP("\\\\'");  ENDRANGE("'");
        APOP();

        APUSH("&code-preprocessor");
            REGEXSUB("(#define[[:space:]]+[a-zA-Z0-9_]+)", 1);
            REGEXSUB("\\b(BEGIN)\\b", 1);
            REGEXSUB("\\b(END)\\b", 1);
            REGEXSUB("\\b(kprobe)\\b", 1);
            REGEXSUB("\\b(kretprobe)\\b", 1);
            REGEXSUB("\\b(uprobe)\\b", 1);
            REGEXSUB("\\b(uretprobe)\\b", 1);
            REGEXSUB("\\b(tracepoint)\\b", 1);
            REGEXSUB("\\b(usdt)\\b", 1);
            REGEXSUB("\\b(profile)\\b", 1);
            REGEXSUB("\\b(interval)\\b", 1);
            REGEXSUB("\\b(software)\\b", 1);
            REGEXSUB("\\b(hardware)\\b", 1);
            REGEXSUB("\\b(watchpoint)\\b", 1);
            REGEXSUB("\\b(asyncwatchpoint)\\b", 1);
            REGEXSUB("(kf\\bunc)\\b", 1);
            REGEXSUB("\\b(kretfunc)\\b", 1);
            REGEXSUB("\\b(iter)\\b", 1);
        APOP();

        APUSH("&code-number");
            REGEXSUB("(^|[^[:alnum:]_])(-?([[:digit:]]+\\.[[:digit:]]*)|(([[:digit:]]*\\.[[:digit:]]+)))"WB, 2);
            REGEXSUB("(^|[^[:alnum:]_])(-?[[:digit:]]+)"WB, 2);
            REGEXSUB("(^|[^[:alnum:]_])(0[xX][0-9a-fA-F]+)"WB, 2);
        APOP();

        APUSH("&code-typename");
            REGEXSUB("(@[0-9A-Za-z_]+)", 1);
        APOP();

        APUSH("&code-control-flow");
            KWD("break");
            KWD("case");
            KWD("continue");
            KWD("default");
            KWD("do");
            KWD("else");
            KWD("for");
            KWD("goto");
            KWD("if");
            KWD("return");
            KWD("switch");
            KWD("while");
            KWD("delete");
            REGEXSUB("^[[:space:]]*([[:alpha:]_][[:alnum:]_]*):", 1);
        APOP();

        APUSH("&code-keyword");
            REGEXSUB("(count)\\(", 1);
            REGEXSUB("(hist)\\(", 1);
            REGEXSUB("(lhist)\\(", 1);
            REGEXSUB("\\b(nsecs)\\b", 1);
            REGEXSUB("(kstack)", 1);
            REGEXSUB("\\b(ustack)\\b", 1);
            REGEXSUB("\\b(pid)\\b", 1);
            REGEXSUB("\\b(tid)\\b", 1);
            REGEXSUB("\\b(uid)\\b", 1);
            REGEXSUB("\\b(gid)\\b", 1);
            REGEXSUB("\\b(nsecs)\\b", 1);
            REGEXSUB("\\b(elapsed)\\b", 1);
            REGEXSUB("\\b(numaid)\\b", 1);
            REGEXSUB("\\b(cpu)\\b", 1);
            REGEXSUB("\\b(comm)\\b", 1);
            REGEXSUB("\\b(kstack)\\b", 1);
            REGEXSUB("\\b(ustack)\\b", 1);
            REGEXSUB("\\b(arg[0-9]+)\\b", 1);
            REGEXSUB("\\b(sarg[0-9]+)\\b", 1);
            REGEXSUB("\\b(retval)\\b", 1);
            REGEXSUB("\\b(func)\\b", 1);
            REGEXSUB("\\b(probe)\\b", 1);
            REGEXSUB("\\b(curtask)\\b", 1);
            REGEXSUB("\\b(rand)\\b", 1);
            REGEXSUB("\\b(cgroup)\\b", 1);
            REGEXSUB("\\b(cpid)\\b", 1);
            REGEXSUB("(\\$[0-9A-Za-z_]+)", 1);
            REGEXSUB("\\b(struct)\\b", 1);
            REGEXSUB("\\b(typedef)\\b", 1);
        APOP();

        APUSH("&code-fn-call");
            REGEXSUB("(printf)\\(", 1);
            REGEXSUB("(time)\\(", 1);
            REGEXSUB("(join)\\(", 1);
            REGEXSUB("(str)\\(", 1);
            REGEXSUB("(ksym)\\(", 1);
            REGEXSUB("(usym)\\(", 1);
            REGEXSUB("(kaddr)\\(", 1);
            REGEXSUB("(uaddr)\\(", 1);
            REGEXSUB("(reg)\\(", 1);
            REGEXSUB("(system)\\(", 1);
            REGEXSUB("(exit)\\(", 1);
            REGEXSUB("(cgroupid)\\(", 1);
            REGEXSUB("(ntop)\\(", 1);
            REGEXSUB("(kstack)\\(", 1);
            REGEXSUB("(ustack)\\(", 1);
            REGEXSUB("(cat)\\(", 1);
            REGEXSUB("(signal)\\(", 1);
            REGEXSUB("(strncmp)\\(", 1);
            REGEXSUB("(override)\\(", 1);
            REGEXSUB("(buf)\\(", 1);
            REGEXSUB("(sizeof)\\(", 1);
            REGEXSUB("(print)\\(", 1);
            REGEXSUB("(strftime)\\(", 1);
            REGEXSUB("(path)\\(", 1);
            REGEXSUB("(uptr)\\(", 1);
            REGEXSUB("(kptr)\\(", 1);
            REGEXSUB("(macaddr)\\(", 1);
            REGEXSUB("(cgroup_path)\\(", 1);
        APOP();
    ENDSYN();

    return 0;
}
