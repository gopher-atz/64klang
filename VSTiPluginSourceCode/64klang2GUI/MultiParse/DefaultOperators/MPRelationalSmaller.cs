using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{
    public class MPRelationalSmaller : MPOperator
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPRelationalSmaller()
            : base("<", PrecedenceRelational, true)
        {
        }

        /// <summary>
        /// Find smaller than
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken)
        {
            if (IsUnary(previousToken))
                return -1;
            if (Regex.IsMatch(expression, @"^\<(?!\<)"))
                return 1;
            return -1;
        }

        /// <summary>
        /// Execute smaller than
        /// </summary>
        /// <param name="output"></param>
        public override void Execute(Stack<object> output)
        {
            // Pop two objects from the stack
            object right = PopOrGet(output);
            object left = PopOrGet(output);
            RelationalSmaller(output, left, right);
        }

        /// <summary>
        /// Relational smaller than
        /// </summary>
        /// <param name="output"></param>
        /// <param name="left"></param>
        /// <param name="right"></param>
        public void RelationalSmaller(Stack<object> output, object left, object right)
        {

            // Default implementation depends on the typecodes
            TypeCode tcl = Type.GetTypeCode(left.GetType());
            TypeCode tcr = Type.GetTypeCode(right.GetType());

            // Depending on the types, calculate the addition
            switch (tcl)
            {

                case TypeCode.Byte:
                    byte bt = (Byte)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(bt < (Byte)right); return;
                        case TypeCode.Char: output.Push(bt < (Char)right); return;
                        case TypeCode.Decimal: output.Push(bt < (Decimal)right); return;
                        case TypeCode.Double: output.Push(bt < (Double)right); return;
                        case TypeCode.Int16: output.Push(bt < (Int16)right); return;
                        case TypeCode.Int32: output.Push(bt < (Int32)right); return;
                        case TypeCode.Int64: output.Push(bt < (Int64)right); return;
                        case TypeCode.SByte: output.Push(bt < (SByte)right); return;
                        case TypeCode.Single: output.Push(bt < (Single)right); return;
                        case TypeCode.UInt16: output.Push(bt < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(bt < (UInt32)right); return;
                        case TypeCode.UInt64: output.Push(bt < (UInt64)right); return;
                    }
                    break;

                case TypeCode.Char:
                    char c = (Char)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(c < (Byte)right); return;
                        case TypeCode.Char: output.Push(c < (Char)right); return;
                        case TypeCode.Decimal: output.Push(c < (Decimal)right); return;
                        case TypeCode.Double: output.Push(c < (Double)right); return;
                        case TypeCode.Int16: output.Push(c < (Int16)right); return;
                        case TypeCode.Int32: output.Push(c < (Int32)right); return;
                        case TypeCode.Int64: output.Push(c < (Int64)right); return;
                        case TypeCode.SByte: output.Push(c < (SByte)right); return;
                        case TypeCode.Single: output.Push(c < (Single)right); return;
                        case TypeCode.UInt16: output.Push(c < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(c < (UInt32)right); return;
                        case TypeCode.UInt64: output.Push(c < (UInt64)right); return;
                    }
                    break;

                case TypeCode.DateTime:
                    DateTime dt = (DateTime)left;
                    switch (tcr)
                    {
                        case TypeCode.DateTime: output.Push(dt < (DateTime)right); return;
                    }
                    break;

                case TypeCode.Decimal:
                    decimal d = (Decimal)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(d < (Byte)right); return;
                        case TypeCode.Char: output.Push(d < (Char)right); return;
                        case TypeCode.Decimal: output.Push(d < (Decimal)right); return;
                        case TypeCode.Int16: output.Push(d < (Int16)right); return;
                        case TypeCode.Int32: output.Push(d < (Int32)right); return;
                        case TypeCode.Int64: output.Push(d < (Int64)right); return;
                        case TypeCode.SByte: output.Push(d < (SByte)right); return;
                        case TypeCode.UInt16: output.Push(d < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(d < (UInt32)right); return;
                        case TypeCode.UInt64: output.Push(d < (UInt64)right); return;
                    }
                    break;

                case TypeCode.Double:
                    double dbl = (Double)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(dbl < (Byte)right); return;
                        case TypeCode.Char: output.Push(dbl < (Char)right); return;
                        case TypeCode.Double: output.Push(dbl < (Double)right); return;
                        case TypeCode.Int16: output.Push(dbl < (Int16)right); return;
                        case TypeCode.Int32: output.Push(dbl < (Int32)right); return;
                        case TypeCode.Int64: output.Push(dbl < (Int64)right); return;
                        case TypeCode.SByte: output.Push(dbl < (SByte)right); return;
                        case TypeCode.Single: output.Push(dbl < (Single)right); return;
                        case TypeCode.UInt16: output.Push(dbl < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(dbl < (UInt32)right); return;
                        case TypeCode.UInt64: output.Push(dbl < (UInt64)right); return;
                    }
                    break;

                case TypeCode.Int16:
                    short s = (Int16)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(s < (Byte)right); return;
                        case TypeCode.Char: output.Push(s < (Char)right); return;
                        case TypeCode.Decimal: output.Push(s < (Decimal)right); return;
                        case TypeCode.Double: output.Push(s < (Double)right); return;
                        case TypeCode.Int16: output.Push(s < (Int16)right); return;
                        case TypeCode.Int32: output.Push(s < (Int32)right); return;
                        case TypeCode.Int64: output.Push(s < (Int64)right); return;
                        case TypeCode.SByte: output.Push(s < (SByte)right); return;
                        case TypeCode.Single: output.Push(s < (Single)right); return;
                        case TypeCode.UInt16: output.Push(s < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(s < (UInt32)right); return;
                    }
                    break;

                case TypeCode.Int32:
                    int i = (Int32)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(i < (Byte)right); return;
                        case TypeCode.Char: output.Push(i < (Char)right); return;
                        case TypeCode.Decimal: output.Push(i < (Decimal)right); return;
                        case TypeCode.Double: output.Push(i < (Double)right); return;
                        case TypeCode.Int16: output.Push(i < (Int16)right); return;
                        case TypeCode.Int32: output.Push(i < (Int32)right); return;
                        case TypeCode.Int64: output.Push(i < (Int64)right); return;
                        case TypeCode.SByte: output.Push(i < (SByte)right); return;
                        case TypeCode.Single: output.Push(i < (Single)right); return;
                        case TypeCode.UInt16: output.Push(i < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(i < (UInt32)right); return;
                    }
                    break;

                case TypeCode.Int64:
                    long l = (Int64)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(l < (Byte)right); return;
                        case TypeCode.Char: output.Push(l < (Char)right); return;
                        case TypeCode.Decimal: output.Push(l < (Decimal)right); return;
                        case TypeCode.Double: output.Push(l < (Double)right); return;
                        case TypeCode.Int16: output.Push(l < (Int16)right); return;
                        case TypeCode.Int32: output.Push(l < (Int32)right); return;
                        case TypeCode.Int64: output.Push(l < (Int64)right); return;
                        case TypeCode.SByte: output.Push(l < (SByte)right); return;
                        case TypeCode.Single: output.Push(l < (Single)right); return;
                        case TypeCode.UInt16: output.Push(l < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(l < (UInt32)right); return;
                    }
                    break;

                case TypeCode.SByte:
                    sbyte sb = (SByte)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(sb < (Byte)right); return;
                        case TypeCode.Char: output.Push(sb < (Char)right); return;
                        case TypeCode.Decimal: output.Push(sb < (Decimal)right); return;
                        case TypeCode.Double: output.Push(sb < (Double)right); return;
                        case TypeCode.Int16: output.Push(sb < (Int16)right); return;
                        case TypeCode.Int32: output.Push(sb < (Int32)right); return;
                        case TypeCode.Int64: output.Push(sb < (Int64)right); return;
                        case TypeCode.SByte: output.Push(sb < (SByte)right); return;
                        case TypeCode.Single: output.Push(sb < (Single)right); return;
                        case TypeCode.UInt16: output.Push(sb < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(sb < (UInt32)right); return;
                    }
                    break;

                case TypeCode.Single:
                    float f = (Single)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(f < (Byte)right); return;
                        case TypeCode.Char: output.Push(f < (Char)right); return;
                        case TypeCode.Double: output.Push(f < (Double)right); return;
                        case TypeCode.Int16: output.Push(f < (Int16)right); return;
                        case TypeCode.Int32: output.Push(f < (Int32)right); return;
                        case TypeCode.Int64: output.Push(f < (Int64)right); return;
                        case TypeCode.SByte: output.Push(f < (SByte)right); return;
                        case TypeCode.Single: output.Push(f < (Single)right); return;
                        case TypeCode.UInt16: output.Push(f < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(f < (UInt32)right); return;
                        case TypeCode.UInt64: output.Push(f < (UInt64)right); return;
                    }
                    break;

                case TypeCode.UInt16:
                    ushort us = (UInt16)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(us < (Byte)right); return;
                        case TypeCode.Char: output.Push(us < (Char)right); return;
                        case TypeCode.Decimal: output.Push(us < (Decimal)right); return;
                        case TypeCode.Double: output.Push(us < (Double)right); return;
                        case TypeCode.Int16: output.Push(us < (Int16)right); return;
                        case TypeCode.Int32: output.Push(us < (Int32)right); return;
                        case TypeCode.Int64: output.Push(us < (Int64)right); return;
                        case TypeCode.SByte: output.Push(us < (SByte)right); return;
                        case TypeCode.Single: output.Push(us < (Single)right); return;
                        case TypeCode.UInt16: output.Push(us < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(us < (UInt32)right); return;
                        case TypeCode.UInt64: output.Push(us < (UInt64)right); return;
                    }
                    break;

                case TypeCode.UInt32:
                    uint ui = (UInt32)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(ui < (Byte)right); return;
                        case TypeCode.Char: output.Push(ui < (Char)right); return;
                        case TypeCode.Decimal: output.Push(ui < (Decimal)right); return;
                        case TypeCode.Double: output.Push(ui < (Double)right); return;
                        case TypeCode.Int16: output.Push(ui < (Int16)right); return;
                        case TypeCode.Int32: output.Push(ui < (Int32)right); return;
                        case TypeCode.Int64: output.Push(ui < (Int64)right); return;
                        case TypeCode.SByte: output.Push(ui < (SByte)right); return;
                        case TypeCode.Single: output.Push(ui < (Single)right); return;
                        case TypeCode.UInt16: output.Push(ui < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(ui < (UInt32)right); return;
                        case TypeCode.UInt64: output.Push(ui < (UInt64)right); return;
                    }
                    break;

                case TypeCode.UInt64:
                    ulong ul = (UInt64)left;
                    switch (tcr)
                    {
                        case TypeCode.Byte: output.Push(ul < (Byte)right); return;
                        case TypeCode.Char: output.Push(ul < (Char)right); return;
                        case TypeCode.Decimal: output.Push(ul < (Decimal)right); return;
                        case TypeCode.Double: output.Push(ul < (Double)right); return;
                        case TypeCode.Single: output.Push(ul < (Single)right); return;
                        case TypeCode.UInt16: output.Push(ul < (UInt16)right); return;
                        case TypeCode.UInt32: output.Push(ul < (UInt32)right); return;
                        case TypeCode.UInt64: output.Push(ul < (UInt64)right); return;
                    }
                    break;
            }

            // Invalid operation
            throw new InvalidOperatorTypesException("<", left, right);
        }
    }
}
