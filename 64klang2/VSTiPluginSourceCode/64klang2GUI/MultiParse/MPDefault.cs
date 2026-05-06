using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MultiParse.Default
{
    public class MPDefault
    {
        /// <summary>
        /// Default data types
        /// </summary>
        public enum DataTypes : uint
        {
            Boolean = 1,
            Byte = 1 << 1,
            Char = 1 << 2,
            Decimal = 1 << 3,
            Double = 1 << 4,
            Int16 = 1 << 5,
            Short = 1 << 5,
            Int32 = 1 << 6,
            Int = 1 << 6,
            Int64 = 1 << 7,
            Long = 1 << 7,
            SByte = 1 << 8,
            Single = 1 << 9,
            Float = 1 << 9,
            String = 1 << 10,
            UInt16 = 1 << 11,
            UShort = 1 << 11,
            UInt32 = 1 << 12,
            UInt = 1 << 12,
            UInt64 = 1 << 13,
            ULong = 1 << 13,
            Variable = 1 << 14,
            None = 0x00000000,
            All = 0xFFFFFFFF
        }

        /// <summary>
        /// Default operators
        /// </summary>
        public enum Operators : uint
        {
            Addition = 1,
            Complement = 1 << 1,
            ConditionalAnd = 1 << 2,
            ConditionalOr = 1 << 3,
            Division = 1 << 4,
            Equality = 1 << 5,
            Inequality = 1 << 6,
            LeftShift = 1 << 7,
            LogicalAnd = 1 << 8,
            LogicalOr = 1 << 9,
            LogicalXOr = 1 << 10,
            Modulo = 1 << 12,
            Multiplication = 1 << 13,
            Negative = 1 << 14,
            Not = 1 << 15,
            Positive = 1 << 16,
            RelationalLarger = 1 << 17,
            RelationalLargerEqual = 1 << 18,
            RelationalSmaller = 1 << 19,
            RelationalSmallerEqual = 1 << 20,
            RightShift = 1 << 21,
            Subtraction = 1 << 22,
            Assignment = 1 << 23,
            None = 0x00000000,
            All = 0xFFFFFFFF
        }

        /// <summary>
        /// Default functions
        /// </summary>
        public enum Functions : uint
        {
            Abs = 1,
            Acos = 1 << 1,
            Asin = 1 << 2,
            Atan = 1 << 3,
            Atan2 = 1 << 4,			
            Ceiling = 1 << 5,
            Cos = 1 << 6,
            Cosh = 1 << 7,
            Exp = 1 << 8,
            Floor = 1 << 9,
            Log = 1 << 10,
            Log10 = 1 << 11,            
            Max = 1 << 12,
            Min = 1 << 13,
            Pow = 1 << 14,
            Round = 1 << 15,
            Sign = 1 << 16,
            Sin = 1 << 17,
            Sqrt = 1 << 18,
            Tan = 1 << 19,
            Tanh = 1 << 20,
            Truncate = 1 << 21,            
            None = 0x00000000,
            All = 0xFFFFFFFF
        }

        /// <summary>
        /// Register a number of default data types in the expression parser
        /// </summary>
        /// <param name="datatypes"></param>
        /// <param name="types"></param>
        public static void RegisterDataTypes(List<MPDataType> datatypeList, DataTypes types)
        {
            // Order is important for these
            if ((types & DataTypes.Int32) == DataTypes.Int32)
                datatypeList.Add(new MPInt32());
            if ((types & DataTypes.UInt32) == DataTypes.UInt32)
                datatypeList.Add(new MPUInt32());
            if ((types & DataTypes.Int64) == DataTypes.Int64)
                datatypeList.Add(new MPInt64());
            if ((types & DataTypes.UInt64) == DataTypes.UInt64)
                datatypeList.Add(new MPUInt64());
            if ((types & DataTypes.SByte) == DataTypes.SByte)
                datatypeList.Add(new MPSByte());
            if ((types & DataTypes.Boolean) == DataTypes.Boolean)
                datatypeList.Add(new MPBoolean());
            if ((types & DataTypes.Decimal) == DataTypes.Decimal)
                datatypeList.Add(new MPDecimal());
            if ((types & DataTypes.Double) == DataTypes.Double)
                datatypeList.Add(new MPDouble());
            if ((types & DataTypes.Single) == DataTypes.Single)
                datatypeList.Add(new MPSingle());
            if ((types & DataTypes.Char) == DataTypes.Char)
                datatypeList.Add(new MPChar());
            if ((types & DataTypes.String) == DataTypes.String)
                datatypeList.Add(new MPString());
            if ((types & DataTypes.Int16) == DataTypes.Int16)
                datatypeList.Add(new MPInt16());
            if ((types & DataTypes.UInt16) == DataTypes.UInt16)
                datatypeList.Add(new MPUInt16());
            if ((types & DataTypes.Byte) == DataTypes.Byte)
                datatypeList.Add(new MPByte());
            if ((types & DataTypes.Variable) == DataTypes.Variable)
                datatypeList.Add(new MPVariable());
        }

        /// <summary>
        /// Register a number of default type casts in the expression parser
        /// </summary>
        /// <param name="operators"></param>
        /// <param name="casts"></param>
        public static void RegisterTypeCasts(List<MPOperator> operatorList, DataTypes casts)
        {
            if ((casts & DataTypes.Boolean) == DataTypes.Boolean)
                operatorList.Add(new MPBooleanCast());
            if ((casts & DataTypes.Byte) == DataTypes.Byte)
                operatorList.Add(new MPByteCast());
            if ((casts & DataTypes.Char) == DataTypes.Char)
                operatorList.Add(new MPCharCast());
            if ((casts & DataTypes.Decimal) == DataTypes.Decimal)
                operatorList.Add(new MPDecimalCast());
            if ((casts & DataTypes.Double) == DataTypes.Double)
                operatorList.Add(new MPDoubleCast());
            if ((casts & DataTypes.Int16) == DataTypes.Int16)
                operatorList.Add(new MPInt16Cast());
            if ((casts & DataTypes.Int32) == DataTypes.Int32)
                operatorList.Add(new MPInt32Cast());
            if ((casts & DataTypes.Int64) == DataTypes.Int64)
                operatorList.Add(new MPInt64Cast());
            if ((casts & DataTypes.SByte) == DataTypes.SByte)
                operatorList.Add(new MPSByteCast());
            if ((casts & DataTypes.Single) == DataTypes.Single)
                operatorList.Add(new MPSingleCast());
            if ((casts & DataTypes.String) == DataTypes.String)
                operatorList.Add(new MPStringCast());
            if ((casts & DataTypes.UInt16) == DataTypes.UInt16)
                operatorList.Add(new MPUInt16Cast());
            if ((casts & DataTypes.UInt32) == DataTypes.UInt32)
                operatorList.Add(new MPUInt32Cast());
            if ((casts & DataTypes.UInt64) == DataTypes.UInt64)
                operatorList.Add(new MPUInt64Cast());
        }

        /// <summary>
        /// Register a number of default operators in the expression parser
        /// </summary>
        /// <param name="operatorList"></param>
        /// <param name="operators"></param>
        public static void RegisterOperators(List<MPOperator> operatorList, Operators operators)
        {
            if ((operators & Operators.LeftShift) == Operators.LeftShift)
                operatorList.Add(new MPLeftShift());
            if ((operators & Operators.RightShift) == Operators.RightShift)
                operatorList.Add(new MPRightShift());
            if ((operators & Operators.RelationalSmallerEqual) == Operators.RelationalSmallerEqual)
                operatorList.Add(new MPRelationalSmallerEqual());
            if ((operators & Operators.RelationalLargerEqual) == Operators.RelationalLargerEqual)
                operatorList.Add(new MPRelationalLargerEqual());
            if ((operators & Operators.RelationalSmaller) == Operators.RelationalSmaller)
                operatorList.Add(new MPRelationalSmaller());
            if ((operators & Operators.RelationalLarger) == Operators.RelationalLarger)
                operatorList.Add(new MPRelationalLarger());
            if ((operators & Operators.ConditionalAnd) == Operators.ConditionalAnd)
                operatorList.Add(new MPConditionalAnd());
            if ((operators & Operators.ConditionalOr) == Operators.ConditionalOr)
                operatorList.Add(new MPConditionalOr());
            if ((operators & Operators.LogicalAnd) == Operators.LogicalAnd)
                operatorList.Add(new MPLogicalAnd());
            if ((operators & Operators.LogicalOr) == Operators.LogicalOr)
                operatorList.Add(new MPLogicalOr());
            if ((operators & Operators.LogicalXOr) == Operators.LogicalXOr)
                operatorList.Add(new MPLogicalXOr());
            if ((operators & Operators.Addition) == Operators.Addition)
                operatorList.Add(new MPAddition());
            if ((operators & Operators.Subtraction) == Operators.Subtraction)
                operatorList.Add(new MPSubtraction());
            if ((operators & Operators.Multiplication) == Operators.Multiplication)
                operatorList.Add(new MPMultiplication());
            if ((operators & Operators.Division) == Operators.Division)
                operatorList.Add(new MPDivision());
            if ((operators & Operators.Equality) == Operators.Equality)
                operatorList.Add(new MPEquality());
            if ((operators & Operators.Inequality) == Operators.Inequality)
                operatorList.Add(new MPInequality());
            if ((operators & Operators.Modulo) == Operators.Modulo)
                operatorList.Add(new MPModulo());
            if ((operators & Operators.Positive) == Operators.Positive)
                operatorList.Add(new MPPositive());
            if ((operators & Operators.Negative) == Operators.Negative)
                operatorList.Add(new MPNegative());
            if ((operators & Operators.Not) == Operators.Not)
                operatorList.Add(new MPNot());
            if ((operators & Operators.Complement) == Operators.Complement)
                operatorList.Add(new MPComplement());
            if ((operators & Operators.Assignment) == Operators.Assignment)
                operatorList.Add(new MPAssignment());
        }

        /// <summary>
        /// Register a number of default functions in the expression parser
        /// </summary>
        /// <param name="functionList"></param>
        /// <param name="functions"></param>
        public static void RegisterFunctions(List<MPFunction> functionList, Functions functions)
        {
            if ((functions & Functions.Abs) == Functions.Abs)
                functionList.Add(new MPAbs());
            if ((functions & Functions.Acos) == Functions.Acos)
                functionList.Add(new MPAcos());
            if ((functions & Functions.Asin) == Functions.Asin)
                functionList.Add(new MPAsin());
            if ((functions & Functions.Atan) == Functions.Atan)
                functionList.Add(new MPAtan());
			if ((functions & Functions.Atan2) == Functions.Atan2)
				functionList.Add(new MPAtan2());			
            if ((functions & Functions.Ceiling) == Functions.Ceiling)
                functionList.Add(new MPCeiling());
            if ((functions & Functions.Cos) == Functions.Cos)
                functionList.Add(new MPCos());
            if ((functions & Functions.Cosh) == Functions.Cosh)
                functionList.Add(new MPCosh());
            if ((functions & Functions.Exp) == Functions.Exp)
                functionList.Add(new MPExp());
            if ((functions & Functions.Floor) == Functions.Floor)
                functionList.Add(new MPFloor());
            if ((functions & Functions.Log) == Functions.Log)
                functionList.Add(new MPLog());
            if ((functions & Functions.Log10) == Functions.Log10)
                functionList.Add(new MPLog10());                
            if ((functions & Functions.Max) == Functions.Max)
                functionList.Add(new MPMax());
            if ((functions & Functions.Min) == Functions.Min)
                functionList.Add(new MPMin());
            if ((functions & Functions.Pow) == Functions.Pow)
                functionList.Add(new MPPow());
            if ((functions & Functions.Round) == Functions.Round)
                functionList.Add(new MPRound());
            if ((functions & Functions.Sign) == Functions.Sign)
                functionList.Add(new MPSign());
            if ((functions & Functions.Sin) == Functions.Sin)
                functionList.Add(new MPSin());
            if ((functions & Functions.Sqrt) == Functions.Sqrt)
                functionList.Add(new MPSqrt());
            if ((functions & Functions.Tan) == Functions.Tan)
                functionList.Add(new MPTan());
            if ((functions & Functions.Tanh) == Functions.Tanh)
                functionList.Add(new MPTanh());
            if ((functions & Functions.Truncate) == Functions.Truncate)
                functionList.Add(new MPTruncate());

            functionList.Add(new MPSqr());
            functionList.Add(new MPLerp());
            functionList.Add(new MPMaxTime());
            functionList.Add(new MPExp2());
            functionList.Add(new MPLog2());
            functionList.Add(new MPRand());
            functionList.Add(new MPPi());
            functionList.Add(new MPTau());
            functionList.Add(new MPIn0());
            functionList.Add(new MPIn1());
            functionList.Add(new MPTautime());
            functionList.Add(new MPIfThen());
            functionList.Add(new MPVFrequency());
            functionList.Add(new MPVNote());
            functionList.Add(new MPVVelocity());
            functionList.Add(new MPVTrigger());
            functionList.Add(new MPVGate());
            functionList.Add(new MPVAftertouch());
			functionList.Add(new MPTriSaw());
			functionList.Add(new MPPulse());
        }
    }
}
