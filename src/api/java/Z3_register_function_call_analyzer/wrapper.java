public static boolean registerFunctionCallAnalyzer(long ctx, Object analyzer) throws Z3Exception
{
    boolean res = INTERNALregisterFunctionCallAnalyzer(ctx, analyzer);
    Z3_error_code err = Z3_error_code.fromInt(INTERNALgetErrorCode(ctx));
    if (err != Z3_error_code.Z3_OK)
        throw new Z3Exception(INTERNALgetErrorMsg(ctx, err.toInt()));
    return res;
}
