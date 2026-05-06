using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPAtan2 : MPFunction
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPAtan2()
            : base("[aA]tan2", false)
        {
        }

        /// <summary>
        /// Execute the atan2 function
        /// </summary>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 2)
                throw new ParseException("Invalid number of arguments for Atan2(). 2 arguments expected.");

            // Pop two objects from the stack
            object right = PopOrGet(output);
            object left = PopOrGet(output);
            Atan2(output, left, right);
        }

        /// <summary>
        /// Arc tangent 2
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void Atan2(Stack<object> output, object left, object right)
        {
            // Only doubles are possible
            double a, b;
            if (CastImplicit(left, out a) && CastImplicit(right, out b))
                output.Push(Math.Atan2(a, b));
            else
                throw new InvalidArgumentTypeException("Atan2()", left, right);
        }
    }
}
