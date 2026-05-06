using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPVAftertouch : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPVAftertouch()
            : base("[vV]aftertouch", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "VAftertouch()");

            output.Push((double)(0.5));
        }
    }
}
