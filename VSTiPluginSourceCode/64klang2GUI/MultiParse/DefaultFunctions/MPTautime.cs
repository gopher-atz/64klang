using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPTautime : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPTautime()
            : base("[tT]autime", false)
        {
        }

        /// <summary>
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            if (arguments != 0)
                throw new InvalidArgumentCountException(0, "Tautime()");

            output.Push((double)(Expression.Time * Math.PI * 2.0));
        }
    }
}
