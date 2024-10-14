using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPPi : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPPi()
            : base("[pP]i", false)
        {
        }

        // need custom match for 0 argument functions 
        public override int Match(string expression, object previousToken)
        {
            int len = base.Match(expression, previousToken);
            //if (len != -1)
            //    len += 2;
            return len;
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "Pi()");

            output.Push((double)(Math.PI));
        }
    }
}
