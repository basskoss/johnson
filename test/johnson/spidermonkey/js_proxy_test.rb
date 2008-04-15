require File.expand_path(File.join(File.dirname(__FILE__), "/../../helper"))

module Johnson
  module SpiderMonkey
    class JSLandProxyTest < Johnson::TestCase
      module AModule
      end
      
      class Foo
        class Inner
        end
        
        def self.bar; 10; end

        attr_accessor :bar
        
        def initialize
          @bar = 10
        end
        
        def x2(x)
          x * 2
        end
        
        def add(*args)
          args.inject { |m,n| m += n }
        end
        
        def xform(arg)
          yield(arg)          
        end
      end
      
      class Indexable
        def initialize
          @store = {}
        end
        
        def [](key)
          @store[key]
        end
        
        def []=(key, value)
          @store[key] = value
        end
        
        def key?(key)
          @store.key?(key)
        end
      end

      def setup
        @context = Johnson::SpiderMonkey::Context.new
        @context.evaluate(Johnson::PRELUDE)
      end
      
      def test_proxies_get_reused
        @context["foo"] = @context["bar"] = Foo.new
        assert_js_equal(true, "foo === bar")
      end

      def test_attributes_get_added_to_ruby
        foo = @context["foo"] = Foo.new
        assert !foo.respond_to?(:johnson)
        @context.evaluate("foo.johnson = 'explode';")
        assert foo.respond_to?(:johnson)
        assert_equal('explode', foo.johnson)
        assert_js_equal('explode', 'foo.johnson')
        assert !Foo.new.respond_to?(:johnson)
      end

      def test_assign_function_as_attribute
        foo = @context["foo"] = Foo.new
        assert !foo.respond_to?(:johnson)
        f = @context.evaluate("foo.johnson = function() { return 'explode'; }")
        assert foo.respond_to?(:johnson)
        assert_equal('explode', foo.johnson)
        assert_js_equal('explode', 'foo.johnson()')
        assert_js_equal(f, 'foo.johnson')
        assert !Foo.new.respond_to?(:johnson)
      end

      def test_assign_function_as_attribute_with_this
        foo = @context["foo"] = Foo.new
        @context.evaluate("foo.ex_squared = function(x) { return this.x2(x); }")
        assert_equal(4, foo.ex_squared(2))
        @context.evaluate("foo.ex_squared = 20;")
        assert_equal(20, foo.ex_squared)
      end

      def test_use_ruby_global_object
        func = @context.evaluate("function(x) { return this.x2(x); }")
        foo  = Foo.new
        assert_equal(4, func.call_using(foo, 2))
      end
      
      def test_proxies_roundtrip
        @context["foo"] = foo = Foo.new
        assert_same(foo, @context.evaluate("foo"))
      end
      
      def test_proxies_classes
        @context["Foo"] = Foo
        assert_same(Foo, @context.evaluate("Foo"))
      end
      
      def test_proxies_modules
        @context["AModule"] = AModule
        assert_same(AModule, @context.evaluate("AModule"))
      end
      
      def test_proxies_hashes
        @context["beatles"] = { "george" => "guitar" }
        assert_equal("guitar", @context.evaluate("beatles['george']"))
      end
      
      def test_getter_calls_0_arity_method
        @context["foo"] = Foo.new
        assert_js_equal(10, "foo.bar")
      end
      
      def test_getter_calls_indexer
        @context["foo"] = indexable = Indexable.new
        indexable["bar"] = 10
        
        assert_js_equal(10, "foo.bar")
      end
      
      def test_getter_returns_nil_for_unknown_properties
        @context["foo"] = Foo.new
        assert_js_equal(nil, "foo.quux")
      end

      def test_setter_calls_key=
        @context["foo"] = foo = Foo.new
        assert_js_equal(42, "foo.bar = 42")
        assert_equal(42, foo.bar)
      end
      
      def test_setter_calls_indexer
        @context["foo"] = indexable = Indexable.new
        assert_js_equal(42, "foo.monkey = 42")
        assert_equal(42, indexable["monkey"])
      end
      
      def test_calls_0_arity_method
        @context["foo"] = Foo.new
        assert_js_equal(10, "foo.bar()")
      end
      
      def test_calls_1_arity_method
        @context["foo"] = Foo.new
        assert_js_equal(10, "foo.x2(5)")
      end
      
      def test_calls_n_arity_method
        @context["foo"] = Foo.new
        assert_js_equal(10, "foo.add(4, 2, 2, 1, 1)")
      end
      
      def test_calls_class_method
        @context["Foo"] = Foo
        assert_js_equal(Foo.bar, "Foo.bar()")
      end
      
      def test_accesses_consts
        @context["Foo"] = Foo
        assert_same(Foo::Inner, @context.evaluate("Foo.Inner"))
      end
            
      def test_can_create_new_instances_in_js
        @context["Foo"] = Foo
        foo = @context.evaluate("Foo.new()")
        assert_kind_of(Foo, foo)
      end
      
      def test_dwims_blocks
        @context["foo"] = Foo.new
        assert_js_equal(4, "foo.xform(2, function(x) { return x * 2 })")
      end    
    end
  end
end
