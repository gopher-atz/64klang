using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPIn1 : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPIn1()
            : base("[iI]n1", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "In1()");

            output.Push((double)Expression.In1);
        }
    }
}
