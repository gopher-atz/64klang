using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace MultiParse
{
    public abstract class MPFunction : MPOperator
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPFunction(string name, bool postfix)
            : base(name, int.MaxValue, postfix)
        {
        }

        /// <summary>
        /// Find the function
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken)
        {
            // Basic implementation for matching a function with a certain name
            Match m = Regex.Match(expression, @"^" + key + @"\(");
            if (m.Success)
                return m.Length - 1;
            return -1;
        }

        /// <summary>
        /// Invalid execution
        /// </summary>
        /// <param name="output"></param>
        sealed public override void Execute(Stack<object> output)
        {
            throw new ParseException("Invalid execution call. Programming error.");
        }

        /// <summary>
        /// Executes the function. The function should pop the arguments from the output stack,
        /// and push the result of the operation back.
        /// </summary>
        /// <param name="output">The output stack</param>
        /// <param name="arguments">The number of arguments passed with the function</param>
        public abstract void Execute(Stack<object> output, int arguments);
    }
}
