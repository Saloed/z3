  protected static native boolean INTERNALregisterFunctionCallAnalyzer(long ctx, int analyzer);


  public static long INTERNALrunFunctionCallAnalyzer(
        long rawCtx, int analyzerId, int functionId, long rawExpression, long[] rawInArgs, long[] rawOutArgs
  ) {
    Context ctx = Context.create(rawCtx);
    Expr expression = Expr.create(ctx, rawExpression);

    Expr[] inArgs = new Expr[rawInArgs.length];
    Expr[] outArgs = new Expr[rawOutArgs.length];
    for (int i = 0; i < rawInArgs.length; i++) {
        inArgs[i] = Expr.create(ctx, rawInArgs[i]);
    }
    for (int i = 0; i < rawOutArgs.length; i++) {
        outArgs[i] = Expr.create(ctx, rawOutArgs[i]);
    }
    Expr result = ctx.runFunctionCallAnalyzer(analyzerId, functionId, expression, inArgs, outArgs);
    return Z3Object.getNativeObject(result);
  }
