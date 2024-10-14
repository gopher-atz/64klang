using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPRound : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPRound()
            : base("[rR]ound", false)
        {
        }

        /// <summary>
        /// Execute rounding
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            object value, precision;

            switch (arguments)
            {
                case 1:
                    value = PopOrGet(output);
                    Round(output, value);
                    break;
                case 2:
                    precision = PopOrGet(output);
                    value = PopOrGet(output);
                    Round(output, value, precision);
                    break;
                default:
                    throw new InvalidArgumentCountException(1, 2, "Round()");
            }
        }

        /// <summary>
        /// Round
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Round(Stack<object> output, object arg)
        {
            // Execute
            double dbl;
            decimal dec;
            bool dblOk = CastImplicit(arg, out dbl);
            bool decOk = CastImplicit(arg, out dec);
            if (dblOk && decOk)
                throw new ParseException("Ambiguous call to Round() for type '" + arg.GetType() + "'");
            if (dblOk)
                output.Push(Math.Round(dbl));
            else if (decOk)
                output.Push(Math.Round(dec));
            else
                throw new InvalidArgumentTypeException("Round()", arg);
        }

        /// <summary>
        /// Round
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void Round(Stack<object> output, object left, object right)
        {
            int precision;
            double dbl, dec;
            if (!CastImplicit(right, out precision))
                throw new InvalidArgumentTypeException("Round()", left, right);

            // Execute
            bool dblOk = CastImplicit(left, out dbl);
            bool decOk = CastImplicit(left, out dec);
            if (dblOk && decOk)
                throw new ParseException("Ambiguous call to Round() for type '" + left.GetType() + "'");
            if (dblOk)
                output.Push(Math.Round(dbl, precision));
            else if (decOk)
                output.Push(Math.Round(dec, precision));
            else
                throw new InvalidArgumentTypeException("Round()", left);
        }
    }
}
