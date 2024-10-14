using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPVFrequency : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPVFrequency()
            : base("[vV]frequency", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "VFrequency()");

            output.Push((double)(44.1));
        }
    }
}
