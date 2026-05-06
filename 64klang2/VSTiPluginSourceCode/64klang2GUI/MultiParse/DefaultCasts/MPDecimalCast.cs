using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPDecimalCast : MPOperator
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPDecimalCast()
            : base("(decimal)", PrecedenceUnary, false)
        {
        }

        /// <summary>
        /// Find byte cast
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken)
        {
            if (!IsUnary(previousToken))
                return -1;
            Match m = Regex.Match(expression, @"^\([dD]ecimal\)");
            if (m.Success)
                return m.Length;
            return -1;
        }

        /// <summary>
        /// Execute byte cast
        /// </summary>
        /// <param name="output"></param>
        public override void Execute(Stack<object> output)
        {
            // Pop object from the stack
            object top = PopOrGet(output);
            TypeCode tc = Type.GetTypeCode(top.GetType());

            switch (tc)
            {
                case TypeCode.Byte: output.Push((decimal)(Byte)top); return;
                case TypeCode.Char: output.Push((decimal)(Char)top); return;
                case TypeCode.Decimal: output.Push((decimal)(Decimal)top); return;
                case TypeCode.Double: output.Push((decimal)(Double)top); return;
                case TypeCode.Int16: output.Push((decimal)(Int16)top); return;
                case TypeCode.Int32: output.Push((decimal)(Int32)top); return;
                case TypeCode.Int64: output.Push((decimal)(Int64)top); return;
                case TypeCode.SByte: output.Push((decimal)(SByte)top); return;
                case TypeCode.Single: output.Push((decimal)(Single)top); return;
                case TypeCode.UInt16: output.Push((decimal)(UInt16)top); return;
                case TypeCode.UInt32: output.Push((decimal)(UInt32)top); return;
                case TypeCode.UInt64: output.Push((decimal)(UInt64)top); return;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("(Decimal)", top);
        }
    }
}
