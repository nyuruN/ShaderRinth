#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <variant>

// Forward declares
class Widget;

// Request to delete a widget, calls onShutdown()
struct DeleteWidget {
  int widget_id;
  DeleteWidget(int widget_id) : widget_id(widget_id) {}
};

// Request to add a widget, calls onStartup()
struct AddWidget {
  std::shared_ptr<Widget> widget;
  AddWidget(std::shared_ptr<Widget> widget) : widget(widget) {}
};

using Event = std::variant< //
    DeleteWidget,           //
    AddWidget               //
    >;

// Global event queue used to communicate certain
// actions to the main Application that the senders
// themselves cannot solve
struct EventQueue {
public:
  static void push(Event event) { instance().queue.push_back(event); }
  static std::optional<Event> pop() {
    if (instance().queue.empty())
      return {};
    Event event = instance().queue.front();
    instance().queue.pop_front();
    return event;
  }

private:
  // Access a static instance of EventQueue
  static EventQueue &instance() {
    static EventQueue instance;
    return instance;
  }

  std::deque<Event> queue;
};
