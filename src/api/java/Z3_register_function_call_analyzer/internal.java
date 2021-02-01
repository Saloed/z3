  protected static native boolean INTERNALregisterFunctionCallAnalyzer(long ctx, Object analyzer);


  protected static long INTERNALrunFunctionCallAnalyzer(
        long rawCtx, Object rawAnalyzer, int functionId, long rawExpression, long[] rawInArgs, long[] rawOutArgs
  ) {
    Context ctx = new Context(rawCtx);
    Expr expression = new Expr(ctx, rawExpression);

    Expr[] inArgs = new Expr[rawInArgs.length];
    Expr[] outArgs = new Expr[rawOutArgs.length];
    for (int i = 0; i < rawInArgs.length; i++) {
        inArgs[i] = new Expr(ctx, rawInArgs[i]);
    }
    for (int i = 0; i < rawOutArgs.length; i++) {
        outArgs[i] = new Expr(ctx, rawOutArgs[i]);
    }

    FunctionCallAnalyzer analyzer = (FunctionCallAnalyzer) rawAnalyzer;
    Expr result = analyzer.analyze(functionId, expression, inArgs, outArgs);

    return result.getNativeObject();
  }

