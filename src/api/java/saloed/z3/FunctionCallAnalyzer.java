package saloed.z3;

public interface FunctionCallAnalyzer {
    Expr analyze(int functionId, Expr expression, Expr[] inArgs, Expr[] outArgs);
}
