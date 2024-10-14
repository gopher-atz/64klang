using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPUInt32Cast : MPOperator
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPUInt32Cast()
            : base("(uint)", PrecedenceUnary, false)
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
            Match m = Regex.Match(expression, @"^\((uint|UInt32)\)");
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
                case TypeCode.Byte: output.Push((uint)(Byte)top); return;
                case TypeCode.Char: output.Push((uint)(Char)top); return;
                case TypeCode.Decimal: output.Push((uint)(Decimal)top); return;
                case TypeCode.Double: output.Push((uint)(Double)top); return;
                case TypeCode.Int16: output.Push((uint)(Int16)top); return;
                case TypeCode.Int32: output.Push((uint)(Int32)top); return;
                case TypeCode.Int64: output.Push((uint)(Int64)top); return;
                case TypeCode.SByte: output.Push((uint)(SByte)top); return;
                case TypeCode.Single: output.Push((uint)(Single)top); return;
                case TypeCode.UInt16: output.Push((uint)(UInt16)top); return;
                case TypeCode.UInt32: output.Push((uint)(UInt32)top); return;
                case TypeCode.UInt64: output.Push((uint)(UInt64)top); return;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("(UInt32)", top);
        }
    }
}
