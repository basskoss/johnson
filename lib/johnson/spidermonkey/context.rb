module Johnson #:nodoc:
  module SpiderMonkey #:nodoc:
    class Context # native
      def initialize(options={})
        initialize_native(options)
        @gcthings = {}
        self["Ruby"] = Object
      end
            
      protected
      
      # called from JSFunction executor - js_proxy.c:call_proc
      def call_proc_by_oid(oid, *args)
        id2ref(oid).call(*args)
      end
      
      # called from JSFunction executor - js_proxy.c:unwrap
      def id2ref(oid)
        ObjectSpace._id2ref(oid)
      end
      
      # called from js_proxy.c:method_missing
      def jsend(target, symbol, args)
        block = args.pop if args.last.is_a?(RubyProxy) && args.last.function?
        target.__send__(symbol, *args, &block)
      end

      # called from js_proxy.c:get
      def autovivified(target, attribute)
        target.send(:__johnson_js_properties)[attribute]
      end

      # called from js_proxy.c:get
      def autovivified?(target, attribute)
        return false unless target.respond_to?(:__johnson_js_properties)
        target.send(:__johnson_js_properties).has_key?(attribute)
      end

      # called from js_proxy.c:set
      def autovivify(target, attribute, value)

        (class << target; self; end).instance_eval do
          unless target.respond_to?(:__johnson_js_properties)
            define_method(:__johnson_js_properties) do
              @__johnson_js_properties ||= {}
            end
          end
          define_method(:"#{attribute}=") do |arg|
            send(:__johnson_js_properties)[attribute] = arg
          end
          define_method(:"#{attribute}") do
            send(:__johnson_js_properties)[attribute]
          end
        end

        target.send(:"#{attribute}=", value)
      end
      
      # called from js_proxy.c:make_js_proxy
      def add_gcthing(thing)
        @gcthings[thing.object_id] = thing
      end
      
      # called from js_proxy.c:finalize
      def remove_gcthing(thing)
        @gcthings.delete(thing.object_id)
      end
    end
  end
end
