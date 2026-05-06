using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using MultiParse.Default;

namespace MultiParse
{
    /// <summary>
    /// Expression
    /// </summary>
    public class Expression
    {
        private delegate void CompiledAction();

        // Necessary variables at evaluation time
        private LinkedList<CompiledAction> compiledQueue;
        private Stack<object> outputStack;
        private Stack<object> operatorStack;

        // added public concatenated RPN string (kolon separated)
        public string RPN;
        // added public Time counter
        public static double Time = 0.0;
        public static double In0 = 0.0;
        public static double In1 = 0.0;

        // Necessary variables at compile time
        private LinkedList<int> argumentCounts;
        private object lastToken;
        private BracketOpen bracketOpen;
        private BracketClose bracketClose;
        private FunctionSeparator functionSeparator;

        // Compilation state
        private bool isCompiled = false;
        private string expression;

        /// <summary>
        /// Gets the list of available operators
        /// Any access to the list will result in a recompilation of the expression
        /// </summary>
        public List<MPOperator> Operators { get { isCompiled = false; return operatorList; } }
        private List<MPOperator> operatorList;

        /// <summary>
        /// Gets the list of available data types
        /// Any access to the list will result in a recompilation of the expression
        /// </summary>
        public List<MPDataType> DataTypes { get { isCompiled = false; return typeList; } }
        private List<MPDataType> typeList;

        /// <summary>
        /// Gets the list of available functions
        /// Any access to the list will result in a recompilation of the expression
        /// </summary>
        public List<MPFunction> Functions { get { isCompiled = false; return functionList; } }
        private List<MPFunction> functionList;

        /// <summary>
        /// Gets or sets the expression that needs to be compiled
        /// Any access will result in a recompilation of the expression
        /// </summary>
        public string ParseExpression
        {
            get
            {
                return expression;
            }
            set
            {
                RPN = "";
                expression = value;
                isCompiled = false;
            }
        }

        /// <summary>
        /// Gets the number of actions that are executed. Returns -1 if the expression was not yet compiled
        /// </summary>
        public int CompiledOperationCount
        {
            get
            {
                if (isCompiled)
                    return compiledQueue.Count;
                return -1;
            }
        }

        /// <summary>
        /// Constructor for an expression with all default data types, type casts, functions and operators
        /// </summary>
        /// <param name="expression"></param>
        public Expression()
            : this(MPDefault.DataTypes.All, MPDefault.Operators.All, MPDefault.Functions.All)
        {
        }

        /// <summary>
        /// Constructor while registering default data types.
        /// The data types will be registered, as well as their respective explicit cast operators.
        /// No additional operators will be registered or functions
        /// </summary>
        /// <param name="datatypes"></param>
        public Expression(MPDefault.DataTypes datatypes)
            : this(datatypes, MPDefault.Operators.None, MPDefault.Functions.None)
        {
        }

        /// <summary>
        /// Constructor while registering default data types.
        /// The data types will be registered, as well as their respective explicit cast operators.
        /// The default operators and functions will be registered in the operator list and function list respectively.
        /// </summary>
        /// <param name="datatypes">A combination of default data types</param>
        /// <param name="operators">A combination of default operators</param>
        /// <param name="functions">A combination of default functions</param>
        public Expression(MPDefault.DataTypes datatypes, MPDefault.Operators operators, MPDefault.Functions functions)
        {
            // Accessible variables
            operatorList = new List<MPOperator>();
            typeList = new List<MPDataType>();
            functionList = new List<MPFunction>();

            // Variables necessary on evaluation time
            compiledQueue = new LinkedList<CompiledAction>();
            outputStack = new Stack<object>();
            operatorStack = new Stack<object>();

            // Variables necessary on compile time
            argumentCounts = new LinkedList<int>();
            lastToken = null;
            isCompiled = false;

            // Set special characters
            SetSpecialCharacters(new BracketOpen(), new BracketClose(), new FunctionSeparator());

            // Add all default types
            MPDefault.RegisterDataTypes(typeList, datatypes);
            MPDefault.RegisterTypeCasts(operatorList, datatypes);
            MPDefault.RegisterOperators(operatorList, operators);
            MPDefault.RegisterFunctions(functionList, functions);
        }

        /// <summary>
        /// Constructor with another expression as a template also copying the parsed expression
        /// </summary>
        /// <param name="e"></param>
        public Expression(Expression e)
        {
            // Accessible variables
            operatorList = new List<MPOperator>(e.operatorList);
            typeList = new List<MPDataType>(e.typeList);
            functionList = new List<MPFunction>(e.functionList);

            // Variables necessary on evaluation time
            isCompiled = e.isCompiled;
            expression = e.expression;
            if (e.isCompiled)
                compiledQueue = new LinkedList<CompiledAction>(e.compiledQueue);
            else
                compiledQueue = new LinkedList<CompiledAction>();
            outputStack = new Stack<object>();
            operatorStack = new Stack<object>();

            // Variables necessary on compile time
            argumentCounts = new LinkedList<int>();
            lastToken = null;

            // Set special characters
            SetSpecialCharacters(e.bracketOpen, e.bracketClose, e.functionSeparator);
        }

        /// <summary>
        /// Specify the special characters used by the expression parser
        /// </summary>
        /// <param name="bracketOpen"></param>
        /// <param name="bracketClose"></param>
        /// <param name="functionSeparator"></param>
        public void SetSpecialCharacters(BracketOpen bracketOpen, BracketClose bracketClose, FunctionSeparator functionSeparator)
        {
            this.bracketOpen = bracketOpen;
            this.bracketClose = bracketClose;
            this.functionSeparator = functionSeparator;
        }

        /// <summary>
        /// Evaluate an expression
        /// </summary>
        /// <param name="expression"></param>
        /// <returns></returns>
        public object Evaluate()
        {
            if (!isCompiled)
                return Compile();
            return Execute();
        }

        /// <summary>
        /// Evaluate an expression
        /// </summary>
        /// <param name="expression"></param>
        /// <returns></returns>
        public object Evaluate(string expression)
        {
            this.expression = expression;
            return Compile();
        }

        /// <summary>
        /// Compile the expression
        /// </summary>
        /// <param name="expression"></param>
        private object Compile()
        {
            if (String.IsNullOrEmpty(expression))
                throw new ParseException("No expression specified.");

            // Clear all necessary stacks etc.
            outputStack.Clear();
            operatorStack.Clear();
            argumentCounts.Clear();
            argumentCounts.AddLast(1);
            compiledQueue.Clear();
            lastToken = null;

            // Compile the expression
            string compileExpression = (string)expression.Clone();
            while (!String.IsNullOrWhiteSpace(compileExpression))
                ProcessToken(ref compileExpression);
            ExecuteStack();            

            // Check the stack
            if (outputStack.Count != 1)
                throw new ParseException("Invalid output stack. Cannot resolve to a determined result.");

            // Set compiled flag to true and finish
            isCompiled = true;
            object result = outputStack.Pop();
            if (result is IMPGettable)
                return ((IMPGettable)result).Get();
            return result;
        }

        /// <summary>
        /// Execute the compiled expressions
        /// </summary>
        /// <returns></returns>
        private object Execute()
        {
            // Create all necessary stacks etc.
            outputStack.Clear();
            operatorStack.Clear();

            // Execute the expression
            foreach (CompiledAction action in compiledQueue)
                action();

            // Check the stack
            if (outputStack.Count != 1)
                throw new ParseException("Invalid output stack. Cannot resolve to a determined result.");
            object result = outputStack.Pop();
            if (result is IMPGettable)
                return ((IMPGettable)result).Get();
            return result;
        }

        /// <summary>
        /// Read a token and process it
        /// </summary>
        /// <param name="expression"></param>
        private void ProcessToken(ref string expression)
        {
            int len = 0;

            // Read a function seperator
            len = functionSeparator.Match(expression, lastToken);
            if (len > 0)
            {
                // Compile to opening bracket or function seperator
                CompileToBracket(false);
                expression = expression.Substring(len);
                argumentCounts.Last.Value++;
                lastToken = functionSeparator;
                return;
            }

            // Read a token that is a function
            foreach (MPFunction fun in functionList)
            {
                len = fun.Match(expression, lastToken);
                if (len > 0)
                {
                    expression = expression.Substring(len);
                    operatorStack.Push(fun);
                    lastToken = fun;
                    return;
                }
            }

            // Read a token that is an operator
            foreach (MPOperator op in operatorList)
            {
                len = op.Match(expression, lastToken);
                if (len > 0)
                {
                    expression = expression.Substring(len);
                    while (true)
                    {
                        // Empty stack means skip
                        if (operatorStack.Count == 0)
                            break;

                        // Find the operator o2 on top of the stack
                        object peek = operatorStack.Peek();
                        if (peek is BracketOpen)
                            break;
                        if (peek is BracketClose)
                            throw new ParseException("Closing parenthesis found on the operator stack. Programming error.");
                        if (peek is FunctionSeparator)
                            throw new ParseException("Function separator found on the operator stack. Programming error.");

                        // If it is an operator
                        MPOperator op2 = peek as MPOperator;
                        if ((op.LeftAssociative && (op.Precedence == op2.Precedence))
                            || (op.Precedence < op2.Precedence))
                        {
                            op2 = operatorStack.Pop() as MPOperator;

                            // Compile
                            CompiledAction action = delegate() { RPN += op2.Key + ":"; op2.Execute(outputStack); };
                            action();
                            compiledQueue.AddLast(action);
                        }
                        else
                            break;
                    }

                    // Push the operator
                    operatorStack.Push(op);
                    lastToken = op;
                    return;
                }
            }

            // Read a token that is a datatype
            foreach (MPDataType dt in typeList)
            {
                object converted = null;
                len = dt.Match(expression, lastToken, out converted);
                if (len > 0)
                {
                    expression = expression.Substring(len);
                    CompiledAction action = delegate() { RPN += converted + ":"; outputStack.Push(converted); };
                    action();
                    compiledQueue.AddLast(action);
                    lastToken = converted;
                    return;
                }
            }

            // Check for a bracket opening
            len = bracketOpen.Match(expression, lastToken);
            if (len > 0)
            {
                expression = expression.Substring(len);
                // Update the arguments stack
                if (expression.StartsWith(")"))
                    argumentCounts.AddLast(0);
                else
                    argumentCounts.AddLast(1);
                operatorStack.Push(bracketOpen);
                lastToken = bracketOpen;
                return;
            }

            // Check for a bracket closing
            len = bracketClose.Match(expression, lastToken);
            if (len > 0)
            {
                // Remove from the expression
                expression = expression.Substring(len);

                // If the last token was a closing bracket, then the number of arguments is zero
                //if (lastToken == bracketOpen)
                //    argumentCounts.RemoveLast();
                //else
                {
                    // Evaluate until the opening bracket
                    CompileToBracket(true);
                    if ((operatorStack.Count > 0) && (operatorStack.Peek() is MPFunction))
                    {
                        int arguments = argumentCounts.Last.Value;
                        MPFunction fun = operatorStack.Pop() as MPFunction;
                        CompiledAction action = delegate() { RPN += fun.Key + ":"; fun.Execute(outputStack, arguments); };
                        action();
                        compiledQueue.AddLast(action);
                        argumentCounts.RemoveLast();
                    }
                    else
                    {
                        if (argumentCounts.Last.Value > 1)
                            throw new ParseException("Invalid function separator found");
                        if (argumentCounts.Last.Value == 0)
                            throw new ParseException("Empty brackets without function found");
                        argumentCounts.RemoveLast();
                    }
                }

                // Finished
                lastToken = bracketClose;
                return;
            }

            // Unknown token, throw an error
            throw new ParseException("Invalid token found for '" + expression + "'");
        }

        /// <summary>
        /// Execute all operators on the operator stack until an opening bracket
        /// </summary>
        private void CompileToBracket(bool popBracket)
        {
            while (true)
            {
                // Empty stack means quit
                if (operatorStack.Count == 0)
                    throw new ParseException("Empty stack. Invalid expression.");

                // Pop the top
                object pop = operatorStack.Pop();
                if (pop == bracketOpen)
                {
                    if (!popBracket)
                        operatorStack.Push(bracketOpen);
                    break;
                }
                if (pop == bracketClose)
                    throw new ParseException("Closing bracket found on the operator stack. Programming error.");
                if (pop is MPFunction)
                    throw new ParseException("Function found without matching brackets.");
                if (pop is MPOperator)
                {
                    MPOperator op = pop as MPOperator;
                    CompiledAction action = delegate() { RPN += op.Key + ":"; op.Execute(outputStack); };
                    action();
                    compiledQueue.AddLast(action);
                }
            }
        }

        /// <summary>
        /// Empty the operator stack
        /// </summary>
        private void ExecuteStack()
        {
            // Show all on the operator stack that is left
            while (operatorStack.Count > 0)
            {
                object pop = operatorStack.Pop();
                if (pop is MPOperator)
                {
                    MPOperator op = pop as MPOperator;
                    CompiledAction action = delegate() { RPN += op.Key + ":"; op.Execute(outputStack); };
                    action();
                    compiledQueue.AddLast(action);
                }
                else
                {
                    if (pop is BracketOpen)
                        throw new ParseException("Parenthesis mismatch. Too many '('");
                    throw new ParseException("Invalid parsing token found on the stack.");
                }
            }
        }

        /// <summary>
        /// Find data types in the expression datatype list
        /// </summary>
        /// <param name="datatype"></param>
        /// <returns></returns>
        public MPDataType[] FindDataType(Type datatype)
        {
            List<MPDataType> result = new List<MPDataType>();
            foreach (MPDataType dt in typeList)
            {
                if (dt.GetType().Equals(datatype))
                    result.Add(dt);
            }
            return result.ToArray();
        }

        /// <summary>
        /// Find operator in the expression operator list
        /// </summary>
        /// <param name="op"></param>
        /// <returns></returns>
        public MPOperator[] FindOperator(Type op)
        {
            List<MPOperator> result = new List<MPOperator>();
            foreach (MPOperator o in operatorList)
            {
                if (o.GetType().Equals(op))
                    result.Add(o);
            }
            return result.ToArray();
        }

        /// <summary>
        /// Find function in the expression function list
        /// </summary>
        /// <param name="fun"></param>
        /// <returns></returns>
        public MPFunction[] FindFunction(Type fun)
        {
            List<MPFunction> result = new List<MPFunction>();
            foreach (MPFunction f in functionList)
            {
                if (f.GetType().Equals(fun))
                    result.Add(f);
            }
            return result.ToArray();
        }
    }
}
