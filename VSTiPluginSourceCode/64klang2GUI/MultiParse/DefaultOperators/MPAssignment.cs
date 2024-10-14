using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPAssignment : MPOperator
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPAssignment()
            : base("=", PrecedenceAssignment, false)
        {
        }

        /// <summary>
        /// Match an expression for an assignment
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken)
        {
            if (Regex.IsMatch(expression, @"^\=(?!\=)"))
                return 1;
            return -1;
        }

        /// <summary>
        /// Execute the assignment
        /// </summary>
        /// <param name="output"></param>
        public override void Execute(Stack<object> output)
        {
            // Pop the right operand off the stack
            object right = PopOrGet(output);

            // Pop the left off the stack
            object left = output.Pop();
            if (left is IMPAssignable)
            {
                ((IMPAssignable)left).Assign(right);

                // Push the left operand back on the stack for possible subsequent assignments
                output.Push(right);
            }
            else
                throw new ParseException("Cannot assign to a non-assignable");
        }
    }
}
