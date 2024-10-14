using System;
using System.Text.RegularExpressions;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPNot : MPOperator
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPNot()
            : base("!", PrecedenceUnary, true)
        {
        }

        /// <summary>
        /// Find unary NOT
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken)
        {
            if (!IsUnary(previousToken))
                return -1;
            if (Regex.IsMatch(expression, @"^\!(?![\!\=])"))
                return 1;
            return -1;
        }

        /// <summary>
        /// Execute unary NOT
        /// </summary>
        /// <param name="output"></param>
        public override void Execute(Stack<object> output)
        {
            // Pop object from the stack
            object top = PopOrGet(output);
            Not(output, top);
        }

        /// <summary>
        /// Not
        /// </summary>
        /// <param name="output"></param>
        /// <param name="operand"></param>
        public void Not(Stack<object> output, object operand)
        {
            if (operand is Boolean)
            {
                output.Push(!(Boolean)operand);
                return;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("!", operand);
        }
    }
}
