using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPByteCast : MPOperator
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MPByteCast()
            : base("(byte)", PrecedenceUnary, false)
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
            Match m = Regex.Match(expression, @"^\([bB]yte\)");
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
                case TypeCode.Byte: output.Push((byte)(Byte)top); return;
                case TypeCode.Char: output.Push((byte)(Char)top); return;
                case TypeCode.Decimal: output.Push((byte)(Decimal)top); return;
                case TypeCode.Double: output.Push((byte)(Double)top); return;
                case TypeCode.Int16: output.Push((byte)(Int16)top); return;
                case TypeCode.Int32: output.Push((byte)(Int32)top); return;
                case TypeCode.Int64: output.Push((byte)(Int64)top); return;
                case TypeCode.SByte: output.Push((byte)(SByte)top); return;
                case TypeCode.Single: output.Push((byte)(Single)top); return;
                case TypeCode.UInt16: output.Push((byte)(UInt16)top); return;
                case TypeCode.UInt32: output.Push((byte)(UInt32)top); return;
                case TypeCode.UInt64: output.Push((byte)(UInt64)top); return;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("(Byte)", top);
        }
    }
}
