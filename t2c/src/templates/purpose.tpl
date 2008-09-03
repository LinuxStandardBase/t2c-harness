///////////////////////////////////////////////////////////////////////////
<%comment%>///////////////////////////////////////////////////////////////////////////
static void test_purpose_<%purpose_number%>()
{
<%define%>
    int tp_in_finally = 0;
    T2C_PUT_STATUS(FALSE);

    test_passed_flag = TRUE;          
    nPurposesTotal++;
    TRACE_NO_JOURNAL("test case: %s, TP number: %d ", tet_pname, tet_thistest);
    fprintf(stderr, ".");

    if (gen_hlinks)
    {
        TRACE(t2c_href_full_tpl_, "tp", tet_thistest, "View source code of this test.");
    }

    TRACE0("<%targets%>--------\n");

<%code%>
BEGIN_FINALLY_SECTION
    tp_in_finally = 1;
<%finally%>
END_FINALLY_SECTION
    TP_RETURN;
<%undef%>
}
