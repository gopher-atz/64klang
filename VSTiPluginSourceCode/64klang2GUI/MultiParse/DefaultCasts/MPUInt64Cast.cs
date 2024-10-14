using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPUInt64Cast : MPOperator
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPUInt64Cast()
            : base("(ulong)", PrecedenceUnary, false)
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
            Match m = Regex.Match(expression, @"^\((ulong|UInt64)\)");
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
                case TypeCode.Byte: output.Push((ulong)(Byte)top); return;
                case TypeCode.Char: output.Push((ulong)(Char)top); return;
                case TypeCode.Decimal: output.Push((ulong)(Decimal)top); return;
                case TypeCode.Double: output.Push((ulong)(Double)top); return;
                case TypeCode.Int16: output.Push((ulong)(Int16)top); return;
                case TypeCode.Int32: output.Push((ulong)(Int32)top); return;
                case TypeCode.Int64: output.Push((ulong)(Int64)top); return;
                case TypeCode.SByte: output.Push((ulong)(SByte)top); return;
                case TypeCode.Single: output.Push((ulong)(Single)top); return;
                case TypeCode.UInt16: output.Push((ulong)(UInt16)top); return;
                case TypeCode.UInt32: output.Push((ulong)(UInt32)top); return;
                case TypeCode.UInt64: output.Push((ulong)(UInt64)top); return;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("(UInt64)", top);
        }
    }
}
