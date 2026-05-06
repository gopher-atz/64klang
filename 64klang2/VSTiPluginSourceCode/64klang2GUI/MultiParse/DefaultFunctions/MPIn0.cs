using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPIn0 : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPIn0()
            : base("[iI]n0", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "In0()");

            output.Push((double)Expression.In0);
        }
    }
}
