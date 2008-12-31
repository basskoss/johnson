module Johnson
  module Visitors
    class EnumeratingVisitor < Visitor
      attr_accessor :block
      def initialize(block)
        @block = block
      end

      def visit_SourceElements(o)
        block.call(o)
        super
      end

      def visit_LexicalScope(o)
        block.call(o)
        super
      end

      %w{
        ArrayLiteral Comma Export FunctionCall Import New ObjectLiteral
        VarStatement LetStatement
      }.each do |type|
        define_method(:"visit_#{type}") do |o|
          block.call(o)
          super
        end
      end

      %w{
        Name Number Regexp String
        Break Continue False Null This True
      }.each do |type|
        define_method(:"visit_#{type}") do |o|
          block.call(o)
          super
        end
      end

      def visit_For(o)
        block.call(o)
        super
      end

      def visit_ForIn(o)
        block.call(o)
        super
      end

      def visit_Try(o)
        block.call(o)
        super
      end

      %w{ Ternary If Catch }.each do |node|
        define_method(:"visit_#{node}") do |o|
          block.call(o)
          super
        end
      end

      ### UNARY NODES ###
      %w{ BitwiseNot Delete Not Parenthesis PostfixDecrement PostfixIncrement
        PrefixDecrement PrefixIncrement Return Throw Typeof UnaryNegative
        UnaryPositive Void
      }.each do |node|
        define_method(:"visit_#{node}") do |o|
          block.call(o)
          super
        end
      end

      ### FUNCTION NODES ###
      def visit_Function(o)
        block.call(o)
        super
      end

      ### BINARY NODES ###
      %w{ And AssignExpr BracketAccess Case Default DoWhile DotAccessor Equal
        GetterProperty GreaterThan GreaterThanOrEqual In InstanceOf Label
        LessThan LessThanOrEqual NotEqual OpAdd OpAddEqual OpBitAnd
        OpBitAndEqual OpBitOr OpBitOrEqual OpBitXor OpBitXorEqual OpDivide
        OpDivideEqual OpEqual OpLShift OpLShiftEqual OpMod OpModEqual
        OpMultiply OpMultiplyEqual OpRShift OpRShiftEqual OpSubtract
        OpSubtractEqual OpURShift OpURShiftEqual Or Property SetterProperty
        StrictEqual StrictNotEqual Switch While With
      }.each do |node|
        define_method(:"visit_#{node}") do |o|
          block.call(o)
          super
        end
      end
    end
  end
end
