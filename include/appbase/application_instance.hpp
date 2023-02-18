#pragma once

namespace appbase {

   using executor_t = appbase_executor;

   class application : public application_base {
   public:
      static application&  instance() {
         if (__builtin_expect(!!app_instance, 1))
            return *app_instance;
         app_instance.reset(new application);
         return *app_instance;
      }

      static void reset_app_singleton() { app_instance.reset(); }
      
      static bool null_app_singleton()  { return !app_instance; }

      template <typename Func>
      auto post( int priority, Func&& func ) {
         return application_base::post(executor_, priority, std::forward<Func>(func));
      }

      void exec() {
         application_base::exec(executor_);
      }

      boost::asio::io_service& get_io_service() {
         return executor_.get_io_service();
      }

      auto& get_priority_queue() {
         return executor_.get_priority_queue();
      }
      
      void startup() {
         application_base::startup(get_io_service());
      }

      application() {
         set_stop_executor_cb([&]() { get_io_service().stop(); });
         set_post_cb([&](int prio, std::function<void()> cb) { this->post(prio, std::move(cb)); });
      }

      executor_t& executor() { return executor_; }

  private:
      inline static std::unique_ptr<application> app_instance;
      executor_t executor_;
   };
}

#include <appbase/plugin.hpp>

namespace appbase {

   template<typename Data, typename DispatchPolicy>
   void channel<Data,DispatchPolicy>::publish(int priority, const Data& data) {
      if (has_subscribers()) {
         // this will copy data into the lambda
         app().post( priority, [this, data]() {
            _signal(data);
         });
      }
   }

   class scoped_app {
   public:
      explicit scoped_app()  { assert(application::null_app_singleton()); app_ = &app(); }
      ~scoped_app() { application::reset_app_singleton(); } // destroy app instance so next instance gets a clean one

      scoped_app(const scoped_app&) = delete;
      scoped_app& operator=(const scoped_app&) = delete;

      // access methods
      application*       operator->()       { return app_; }
      const application* operator->() const { return app_; }

   private:
      application* app_;
   };

   static application& app() { return application::instance(); }
   
}

