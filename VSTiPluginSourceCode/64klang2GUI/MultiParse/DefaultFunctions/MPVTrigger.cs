using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPVTrigger : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPVTrigger()
            : base("[vV]trigger", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "VTrigger()");

            output.Push((double)(0.0));
        }
    }
}
