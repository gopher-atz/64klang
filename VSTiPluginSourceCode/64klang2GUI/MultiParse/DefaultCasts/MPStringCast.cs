using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPStringCast : MPOperator
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPStringCast()
            : base("(string)", PrecedenceUnary, false)
        {
        }

        /// <summary>
        /// Find byte cast
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken)
        {
            if (!IsUnary(previousToken))
                return -1;
            Match m = Regex.Match(expression, @"^\([sS]tring\)");
            if (m.Success)
                return m.Length;
            return -1;
        }

        /// <summary>
        /// Execute byte cast
        /// </summary>
        /// <param name="output"></param>
        public override void Execute(Stack<object> output)
        {
            // Pop object from the stack
            object top = PopOrGet(output);
            if (top is string)
            {
                output.Push((string)(String)top);
                return;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("(String)", top);
        }
    }
}
