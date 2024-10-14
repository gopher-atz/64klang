using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPUInt16Cast : MPOperator
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPUInt16Cast()
            : base("(ushort)", PrecedenceUnary, false)
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
            Match m = Regex.Match(expression, @"^\((ushort|UInt16)\)");
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
                case TypeCode.Byte: output.Push((ushort)(Byte)top); return;
                case TypeCode.Char: output.Push((ushort)(Char)top); return;
                case TypeCode.Decimal: output.Push((ushort)(Decimal)top); return;
                case TypeCode.Double: output.Push((ushort)(Double)top); return;
                case TypeCode.Int16: output.Push((ushort)(Int16)top); return;
                case TypeCode.Int32: output.Push((ushort)(Int32)top); return;
                case TypeCode.Int64: output.Push((ushort)(Int64)top); return;
                case TypeCode.SByte: output.Push((ushort)(SByte)top); return;
                case TypeCode.Single: output.Push((ushort)(Single)top); return;
                case TypeCode.UInt16: output.Push((ushort)(UInt16)top); return;
                case TypeCode.UInt32: output.Push((ushort)(UInt32)top); return;
                case TypeCode.UInt64: output.Push((ushort)(UInt64)top); return;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("(UInt16)", top);
        }
    }
}
