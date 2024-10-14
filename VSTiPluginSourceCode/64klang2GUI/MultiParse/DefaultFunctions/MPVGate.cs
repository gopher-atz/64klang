using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPVGate : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPVGate()
            : base("[vV]gate", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "VGate()");

            output.Push((double)(1.0));
        }
    }
}
