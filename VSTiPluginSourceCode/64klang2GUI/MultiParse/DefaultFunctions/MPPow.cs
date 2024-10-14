using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPPow : MPFunction
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPPow()
            : base("[pP]ow", false)
        {
        }

        /// <summary>
        /// Execute the atan2 function
        /// </summary>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 2)
                throw new ParseException("Invalid number of arguments for Pow(). 2 arguments expected.");

            // Pop two objects from the stack
            object right = PopOrGet(output);
            object left = PopOrGet(output);
            Pow(output, left, right);
        }

        /// <summary>
        /// Power
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void Pow(Stack<object> output, object left, object right)
        {
            // Only doubles are possible
            double a, b;
            if (CastImplicit(left, out a) && CastImplicit(right, out b))
                output.Push(Math.Pow(a, b));
            else
                throw new InvalidArgumentTypeException("Pow()", left, right);
        }
    }
}
