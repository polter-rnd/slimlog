// SlimLog
#include "slimlog/logger.h"
#include "slimlog/sinks/ostream_sink.h"

// Test helpers
#include "helpers/common.h"
#include "helpers/stream_capturer.h"

#include <mettle.hpp>

#include <memory>
#include <string>

// IWYU pragma: no_include <utility>
// IWYU pragma: no_include <sstream>
// clazy:excludeall=non-pod-global-static

namespace {

using namespace mettle;
using namespace SlimLog;

const suite<SLIMLOG_CHAR_TYPES> Hierarchy("hierarchy", type_only, [](auto& _) {
    using Char = mettle::fixture_type_t<decltype(_)>;
    using String = std::basic_string<Char>;

    // Test dynamic parent changes
    _.test("parent_changes", []() {
        StreamCapturer<Char> cap_root;
        StreamCapturer<Char> cap_parent1;
        StreamCapturer<Char> cap_parent2;
        StreamCapturer<Char> cap_child;

        const auto pattern = from_utf8<Char>("[{category}] {message}");
        const auto message = from_utf8<Char>("Test message");

        // Create multiple potential parents and a child without parent
        auto root_log = Logger<String>::create(from_utf8<Char>("root"));
        auto parent1_log = Logger<String>::create(root_log, from_utf8<Char>("parent1"));
        auto parent2_log = Logger<String>::create(root_log, from_utf8<Char>("parent2"));
        auto child_log = Logger<String>::create(from_utf8<Char>("child"));

        // Add sinks to all loggers
        root_log->template add_sink<OStreamSink>(cap_root, pattern);
        parent1_log->template add_sink<OStreamSink>(cap_parent1, pattern);
        parent2_log->template add_sink<OStreamSink>(cap_parent2, pattern);
        child_log->template add_sink<OStreamSink>(cap_child, pattern);

        // Check initial parent relationships
        expect(root_log->parent(), equal_to(nullptr));
        expect(child_log->parent(), equal_to(nullptr));
        expect(parent1_log->parent(), equal_to(root_log));
        expect(parent2_log->parent(), equal_to(root_log));

        // Initially, child has no parent
        child_log->info(message);
        expect(cap_child.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));
        expect(cap_parent1.read(), equal_to(String{}));
        expect(cap_parent2.read(), equal_to(String{}));
        expect(cap_root.read(), equal_to(String{}));

        // Set parent1 as parent
        child_log->set_parent(parent1_log);
        expect(child_log->parent(), equal_to(parent1_log));
        child_log->info(message);
        expect(cap_child.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));
        expect(cap_parent1.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));
        expect(cap_parent2.read(), equal_to(String{}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));

        // Change parent to parent2
        child_log->set_parent(parent2_log);
        expect(child_log->parent(), equal_to(parent2_log));
        child_log->info(message);
        expect(cap_child.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));
        expect(cap_parent1.read(), equal_to(String{})); // No longer gets messages
        expect(cap_parent2.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));

        // Make child orphaned again
        child_log->set_parent(nullptr);
        expect(child_log->parent(), equal_to(nullptr));
        child_log->info(message);
        expect(cap_child.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));
        expect(cap_parent1.read(), equal_to(String{}));
        expect(cap_parent2.read(), equal_to(String{}));
        expect(cap_root.read(), equal_to(String{}));

        // Set root as direct parent
        child_log->set_parent(root_log);
        expect(child_log->parent(), equal_to(root_log));
        child_log->info(message);
        expect(cap_child.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));
        expect(cap_parent1.read(), equal_to(String{}));
        expect(cap_parent2.read(), equal_to(String{}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));
    });

    // Test propagation control
    _.test("propagation_control", []() {
        StreamCapturer<Char> cap_root;
        StreamCapturer<Char> cap_parent;
        StreamCapturer<Char> cap_child;
        StreamCapturer<Char> cap_grandchild;

        const auto pattern = from_utf8<Char>("[{category}] {message}");
        const auto message = from_utf8<Char>("Propagation test message");

        // Create a deep hierarchy: root -> parent -> child -> grandchild
        auto root_log = Logger<String>::create(from_utf8<Char>("root"));
        auto parent_log = Logger<String>::create(root_log, from_utf8<Char>("parent"));
        auto child_log = Logger<String>::create(parent_log, from_utf8<Char>("child"));
        auto grandchild_log = Logger<String>::create(child_log, from_utf8<Char>("grandchild"));

        // Add sinks to all loggers
        root_log->template add_sink<OStreamSink>(cap_root, pattern);
        parent_log->template add_sink<OStreamSink>(cap_parent, pattern);
        child_log->template add_sink<OStreamSink>(cap_child, pattern);
        grandchild_log->template add_sink<OStreamSink>(cap_grandchild, pattern);

        // By default, messages propagate all the way up
        grandchild_log->info(message);
        expect(
            cap_grandchild.read(),
            equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(cap_child.read(), equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(
            cap_parent.read(), equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));

        // Stop propagation at child level
        child_log->set_propagate(false);
        grandchild_log->info(message);
        expect(
            cap_grandchild.read(),
            equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(cap_child.read(), equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(cap_parent.read(), equal_to(String{}));
        expect(cap_root.read(), equal_to(String{}));

        // Stop propagation at grandchild level
        grandchild_log->set_propagate(false);
        child_log->set_propagate(true); // Re-enable at child level
        grandchild_log->info(message);
        expect(
            cap_grandchild.read(),
            equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(cap_child.read(), equal_to(String{}));
        expect(cap_parent.read(), equal_to(String{}));
        expect(cap_root.read(), equal_to(String{}));

        // Test middle-level propagation block
        grandchild_log->set_propagate(true);
        child_log->set_propagate(true);
        parent_log->set_propagate(false);
        grandchild_log->info(message);
        expect(
            cap_grandchild.read(),
            equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(cap_child.read(), equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(
            cap_parent.read(), equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(String{}));

        // Re-enable all propagation
        parent_log->set_propagate(true);
        grandchild_log->info(message);
        expect(
            cap_grandchild.read(),
            equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(cap_child.read(), equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(
            cap_parent.read(), equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[grandchild] ") + message + Char{'\n'}));
    });

    // Test complex hierarchies
    _.test("complex_hierarchy", []() {
        // Create a more complex tree structure:
        //           root
        //          /    \
        //     branch1   branch2
        //     /    \      /   \
        // leaf1   leaf2 leaf3 leaf4

        const auto pattern = from_utf8<Char>("[{category}] {message}");
        const auto message = from_utf8<Char>("Complex hierarchy test");

        StreamCapturer<Char> cap_root;
        StreamCapturer<Char> cap_branch1;
        StreamCapturer<Char> cap_branch2;
        StreamCapturer<Char> cap_leaf1;
        StreamCapturer<Char> cap_leaf2;
        StreamCapturer<Char> cap_leaf3;
        StreamCapturer<Char> cap_leaf4;

        auto root_log = Logger<String>::create(from_utf8<Char>("root"));
        auto branch1_log = Logger<String>::create(root_log, from_utf8<Char>("branch1"));
        auto branch2_log = Logger<String>::create(root_log, from_utf8<Char>("branch2"));
        auto leaf1_log = Logger<String>::create(branch1_log, from_utf8<Char>("leaf1"));
        auto leaf2_log = Logger<String>::create(branch1_log, from_utf8<Char>("leaf2"));
        auto leaf3_log = Logger<String>::create(branch2_log, from_utf8<Char>("leaf3"));
        auto leaf4_log = Logger<String>::create(branch2_log, from_utf8<Char>("leaf4"));

        // Add sinks to all loggers
        root_log->template add_sink<OStreamSink>(cap_root, pattern);
        branch1_log->template add_sink<OStreamSink>(cap_branch1, pattern);
        branch2_log->template add_sink<OStreamSink>(cap_branch2, pattern);
        leaf1_log->template add_sink<OStreamSink>(cap_leaf1, pattern);
        leaf2_log->template add_sink<OStreamSink>(cap_leaf2, pattern);
        leaf3_log->template add_sink<OStreamSink>(cap_leaf3, pattern);
        leaf4_log->template add_sink<OStreamSink>(cap_leaf4, pattern);

        // Check parent relationships
        expect(branch1_log->parent(), equal_to(root_log));
        expect(branch2_log->parent(), equal_to(root_log));
        expect(leaf1_log->parent(), equal_to(branch1_log));
        expect(leaf2_log->parent(), equal_to(branch1_log));
        expect(leaf3_log->parent(), equal_to(branch2_log));
        expect(leaf4_log->parent(), equal_to(branch2_log));

        // Test propagation within branch1
        leaf1_log->info(message);
        expect(cap_leaf1.read(), equal_to(from_utf8<Char>("[leaf1] ") + message + Char{'\n'}));
        expect(cap_branch1.read(), equal_to(from_utf8<Char>("[leaf1] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[leaf1] ") + message + Char{'\n'}));
        expect(cap_branch2.read(), equal_to(String{}));
        expect(cap_leaf2.read(), equal_to(String{}));
        expect(cap_leaf3.read(), equal_to(String{}));
        expect(cap_leaf4.read(), equal_to(String{}));

        // Test propagation within branch2
        leaf3_log->info(message);
        expect(cap_leaf3.read(), equal_to(from_utf8<Char>("[leaf3] ") + message + Char{'\n'}));
        expect(cap_branch2.read(), equal_to(from_utf8<Char>("[leaf3] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[leaf3] ") + message + Char{'\n'}));
        expect(cap_branch1.read(), equal_to(String{}));
        expect(cap_leaf1.read(), equal_to(String{}));
        expect(cap_leaf2.read(), equal_to(String{}));
        expect(cap_leaf4.read(), equal_to(String{}));

        // Test cross-branch reparenting
        // Move leaf2 from branch1 to branch2
        leaf2_log->set_parent(branch2_log);
        expect(leaf2_log->parent(), equal_to(branch2_log));
        leaf2_log->info(message);
        expect(cap_leaf2.read(), equal_to(from_utf8<Char>("[leaf2] ") + message + Char{'\n'}));
        expect(
            cap_branch2.read(),
            equal_to(from_utf8<Char>("[leaf2] ") + message + Char{'\n'})); // Now gets messages
        expect(cap_root.read(), equal_to(from_utf8<Char>("[leaf2] ") + message + Char{'\n'}));
        expect(cap_branch1.read(), equal_to(String{})); // No longer gets messages
        expect(cap_leaf1.read(), equal_to(String{}));
        expect(cap_leaf3.read(), equal_to(String{}));
        expect(cap_leaf4.read(), equal_to(String{}));

        // Test branch level propagation control
        branch1_log->set_propagate(false);
        leaf1_log->info(message);
        expect(cap_leaf1.read(), equal_to(from_utf8<Char>("[leaf1] ") + message + Char{'\n'}));
        expect(cap_branch1.read(), equal_to(from_utf8<Char>("[leaf1] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(String{})); // No longer gets messages from branch1

        // Verify branch2 still propagates to root
        leaf3_log->info(message);
        expect(cap_leaf3.read(), equal_to(from_utf8<Char>("[leaf3] ") + message + Char{'\n'}));
        expect(cap_branch2.read(), equal_to(from_utf8<Char>("[leaf3] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[leaf3] ") + message + Char{'\n'}));
    });

    // Test level filtering across hierarchy
    _.test("level_filtering", []() {
        StreamCapturer<Char> cap_root;
        StreamCapturer<Char> cap_parent;
        StreamCapturer<Char> cap_child;

        const auto pattern = from_utf8<Char>("[{category}:{level}] {message}");
        const auto message = from_utf8<Char>("Level test message");

        // Create hierarchy with different levels
        auto root_log = Logger<String>::create(from_utf8<Char>("root"), Level::Warning);
        auto parent_log = Logger<String>::create(root_log, from_utf8<Char>("parent"), Level::Info);
        auto child_log = Logger<String>::create(parent_log, from_utf8<Char>("child"), Level::Debug);

        // Add sinks to all loggers
        root_log->template add_sink<OStreamSink>(cap_root, pattern);
        parent_log->template add_sink<OStreamSink>(cap_parent, pattern);
        child_log->template add_sink<OStreamSink>(cap_child, pattern);

        // Test debug message from child - should propagate to all loggers regardless of their
        // levels since the level check only happens at the logger where log method is called
        child_log->debug(message);
        expect(
            cap_child.read(), equal_to(from_utf8<Char>("[child:DEBUG] ") + message + Char{'\n'}));
        expect(
            cap_parent.read(), equal_to(from_utf8<Char>("[child:DEBUG] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[child:DEBUG] ") + message + Char{'\n'}));

        // Test info message from child - should also propagate to all loggers
        child_log->info(message);
        expect(cap_child.read(), equal_to(from_utf8<Char>("[child:INFO] ") + message + Char{'\n'}));
        expect(
            cap_parent.read(), equal_to(from_utf8<Char>("[child:INFO] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[child:INFO] ") + message + Char{'\n'}));

        // Now test level filtering when calling the logger directly

        // Debug messages to parent should be filtered (parent's level is Info)
        parent_log->debug(message);
        expect(cap_parent.read(), equal_to(String{})); // Filtered out by parent's level
        expect(cap_root.read(), equal_to(String{})); // Not propagated since it was filtered

        // Info messages to parent should pass through
        parent_log->info(message);
        expect(
            cap_parent.read(), equal_to(from_utf8<Char>("[parent:INFO] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[parent:INFO] ") + message + Char{'\n'}));

        // Debug messages to root should be filtered (root's level is Warning)
        root_log->debug(message);
        expect(cap_root.read(), equal_to(String{})); // Filtered out by root's level

        // Info messages to root should be filtered (root's level is Warning)
        root_log->info(message);
        expect(cap_root.read(), equal_to(String{})); // Filtered out by root's level

        // Warning messages to root should pass through
        root_log->warning(message);
        expect(cap_root.read(), equal_to(from_utf8<Char>("[root:WARN] ") + message + Char{'\n'}));

        // Test child logger's level affects its own messages
        child_log->set_level(Level::Error);

        // Debug messages should now be filtered at child level
        child_log->debug(message);
        expect(cap_child.read(), equal_to(String{})); // Filtered by child's new level
        expect(cap_parent.read(), equal_to(String{})); // Not propagated
        expect(cap_root.read(), equal_to(String{})); // Not propagated

        // Error messages should still pass through
        child_log->error(message);
        expect(
            cap_child.read(), equal_to(from_utf8<Char>("[child:ERROR] ") + message + Char{'\n'}));
        expect(
            cap_parent.read(), equal_to(from_utf8<Char>("[child:ERROR] ") + message + Char{'\n'}));
        expect(cap_root.read(), equal_to(from_utf8<Char>("[child:ERROR] ") + message + Char{'\n'}));
    });

    // Test dynamic hierarchy restructuring with sink propagation
    _.test("dynamic_restructuring", []() {
        StreamCapturer<Char> cap1;
        StreamCapturer<Char> cap2;
        StreamCapturer<Char> cap3;

        const auto message = from_utf8<Char>("Restructuring test");

        // Create multiple loggers
        auto root1 = Logger<String>::create(from_utf8<Char>("root1"));
        auto root2 = Logger<String>::create(from_utf8<Char>("root2"));
        auto child = Logger<String>::create(from_utf8<Char>("child"));

        // Add sinks to roots
        auto sink1 = root1->template add_sink<OStreamSink>(cap1);
        auto sink2 = root2->template add_sink<OStreamSink>(cap2);
        auto sink3 = child->template add_sink<OStreamSink>(cap3);

        // Initially child has no parent
        child->info(message);
        expect(cap1.read(), equal_to(String{}));
        expect(cap2.read(), equal_to(String{}));
        expect(cap3.read(), equal_to(message + Char{'\n'}));

        // Set root1 as parent
        child->set_parent(root1);
        expect(child->parent(), equal_to(root1));
        child->info(message);
        expect(cap1.read(), equal_to(message + Char{'\n'}));
        expect(cap2.read(), equal_to(String{}));
        expect(cap3.read(), equal_to(message + Char{'\n'}));

        // Remove sink from root1
        root1->remove_sink(sink1);
        child->info(message);
        expect(cap1.read(), equal_to(String{}));
        expect(cap2.read(), equal_to(String{}));
        expect(cap3.read(), equal_to(message + Char{'\n'}));

        // Change parent to root2
        child->set_parent(root2);
        expect(child->parent(), equal_to(root2));
        child->info(message);
        expect(cap1.read(), equal_to(String{}));
        expect(cap2.read(), equal_to(message + Char{'\n'}));
        expect(cap3.read(), equal_to(message + Char{'\n'}));

        // Add sink back to root1, shouldn't affect child anymore
        root1->template add_sink<OStreamSink>(cap1);
        child->info(message);
        expect(cap1.read(), equal_to(String{}));
        expect(cap2.read(), equal_to(message + Char{'\n'}));
        expect(cap3.read(), equal_to(message + Char{'\n'}));

        // Disable child's sink
        child->set_sink_enabled(sink3, false);
        child->info(message);
        expect(cap1.read(), equal_to(String{}));
        expect(cap2.read(), equal_to(message + Char{'\n'}));
        expect(cap3.read(), equal_to(String{}));

        // Set child as orphan again and re-enable sink
        child->set_parent(nullptr);
        expect(child->parent(), equal_to(nullptr));
        child->set_sink_enabled(sink3, true);
        child->info(message);
        expect(cap1.read(), equal_to(String{}));
        expect(cap2.read(), equal_to(String{}));
        expect(cap3.read(), equal_to(message + Char{'\n'}));
    });

    // Test circular parent references (should be prevented)
    _.test("circular_references", []() {
        const auto message = from_utf8<Char>("Circular reference test");

        // Create loggers
        auto logger1 = Logger<String>::create(from_utf8<Char>("logger1"));
        auto logger2 = Logger<String>::create(logger1, from_utf8<Char>("logger2"));
        auto logger3 = Logger<String>::create(logger2, from_utf8<Char>("logger3"));

        StreamCapturer<Char> cap1;
        StreamCapturer<Char> cap2;
        StreamCapturer<Char> cap3;

        logger1->template add_sink<OStreamSink>(cap1);
        logger2->template add_sink<OStreamSink>(cap2);
        logger3->template add_sink<OStreamSink>(cap3);

        // Normal logging works
        logger3->info(message);
        expect(cap3.read(), equal_to(message + Char{'\n'}));
        expect(cap2.read(), equal_to(message + Char{'\n'}));
        expect(cap1.read(), equal_to(message + Char{'\n'}));

        // Attempt to create a circular reference
        // This shouldn't cause infinite loops in message propagation
        logger1->set_parent(logger3);

        // Make sure logging still works without infinite recursion
        logger3->info(message);
        expect(cap3.read(), equal_to(message + Char{'\n'}));
        expect(cap2.read(), equal_to(message + Char{'\n'}));
        expect(cap1.read(), equal_to(message + Char{'\n'}));

        // Reset hierarchy to normal
        logger1->set_parent(nullptr);
    });

    // Test multiple children with parent changes
    _.test("multiple_children", []() {
        StreamCapturer<Char> cap_parent1;
        StreamCapturer<Char> cap_parent2;
        StreamCapturer<Char> cap_child1;
        StreamCapturer<Char> cap_child2;
        StreamCapturer<Char> cap_child3;

        const auto message = from_utf8<Char>("Multiple children test");

        // Create two parent loggers
        auto parent1 = Logger<String>::create(from_utf8<Char>("parent1"));
        auto parent2 = Logger<String>::create(from_utf8<Char>("parent2"));

        // Create three child loggers, all initially with parent1
        auto child1 = Logger<String>::create(parent1, from_utf8<Char>("child1"));
        auto child2 = Logger<String>::create(parent1, from_utf8<Char>("child2"));
        auto child3 = Logger<String>::create(parent1, from_utf8<Char>("child3"));

        // Add sinks to all loggers
        parent1->template add_sink<OStreamSink>(cap_parent1);
        parent2->template add_sink<OStreamSink>(cap_parent2);
        child1->template add_sink<OStreamSink>(cap_child1);
        child2->template add_sink<OStreamSink>(cap_child2);
        child3->template add_sink<OStreamSink>(cap_child3);

        // All children log to parent1
        child1->info(message);
        child2->info(message);
        child3->info(message);
        expect(
            cap_parent1.read(),
            equal_to(message + Char{'\n'} + message + Char{'\n'} + message + Char{'\n'}));
        expect(cap_parent2.read(), equal_to(String{}));

        // Move child2 and child3 to parent2
        child2->set_parent(parent2);
        expect(child2->parent(), equal_to(parent2));
        child3->set_parent(parent2);
        expect(child3->parent(), equal_to(parent2));

        // Now each child logs to its respective parent
        child1->info(message);
        child2->info(message);
        child3->info(message);
        expect(cap_parent1.read(), equal_to(message + Char{'\n'})); // Only child1's message
        expect(
            cap_parent2.read(),
            equal_to(message + Char{'\n'} + message + Char{'\n'})); // child2 and child3

        // Move all children back to parent1
        child2->set_parent(parent1);
        expect(child2->parent(), equal_to(parent1));
        child3->set_parent(parent1);
        expect(child3->parent(), equal_to(parent1));

        // All children log to parent1 again
        child1->info(message);
        child2->info(message);
        child3->info(message);
        expect(
            cap_parent1.read(),
            equal_to(message + Char{'\n'} + message + Char{'\n'} + message + Char{'\n'}));
        expect(cap_parent2.read(), equal_to(String{}));
    });

    // Test overriding parent sink in children
    _.test("override_sink", []() {
        StreamCapturer<Char> cap;

        const auto pattern = from_utf8<Char>("[{category}] {message}");
        const auto message = from_utf8<Char>("Sink disable propagation test");

        auto parent = Logger<String>::create(from_utf8<Char>("parent"));
        auto child = Logger<String>::create(parent, from_utf8<Char>("child"));

        // Add same sink to both loggers
        auto sink = parent->template add_sink<OStreamSink>(cap, pattern);
        child->add_sink(sink);

        // Child should receive the message
        child->info(message);
        expect(cap.read(), equal_to(from_utf8<Char>("[child] ") + message + Char{'\n'}));
        parent->info(message);
        expect(cap.read(), equal_to(from_utf8<Char>("[parent] ") + message + Char{'\n'}));

        // Disable sink in child
        child->set_sink_enabled(sink, false);

        // Now only parent should receive the message, child should not
        parent->info(message);
        expect(cap.read(), equal_to(from_utf8<Char>("[parent] ") + message + Char{'\n'}));
        child->info(message);
        expect(cap.read(), equal_to(String{}));
    });

    // Test deep sink propagation through multiple hierarchy levels
    _.test("deep_sink_propagation", []() {
        StreamCapturer<Char> cap_sink1;
        StreamCapturer<Char> cap_sink2;
        StreamCapturer<Char> cap_child1;
        StreamCapturer<Char> cap_grandchild1;
        StreamCapturer<Char> cap_grandchild2;
        StreamCapturer<Char> cap_great_grandchild1;
        StreamCapturer<Char> cap_great_grandchild2;

        const auto pattern = from_utf8<Char>("[{category}] {message}");
        const auto message = from_utf8<Char>("Deep propagation test");

        // Create a deep hierarchy tree structure:
        //                    root
        //                   /    \
        //              child1   child2
        //             /      \
        //       grandchild1  grandchild2
        //       /         \
        // great_grandchild1  great_grandchild2
        auto root = Logger<String>::create(from_utf8<Char>("root"));
        auto child1 = Logger<String>::create(root, from_utf8<Char>("child1"));
        auto child2 = Logger<String>::create(root, from_utf8<Char>("child2"));
        auto grandchild1 = Logger<String>::create(child1, from_utf8<Char>("grandchild1"));
        auto grandchild2 = Logger<String>::create(child1, from_utf8<Char>("grandchild2"));
        auto great_grandchild1
            = Logger<String>::create(grandchild1, from_utf8<Char>("great_grandchild1"));
        auto great_grandchild2
            = Logger<String>::create(grandchild1, from_utf8<Char>("great_grandchild2"));

        // Add individual sinks for verification
        child1->template add_sink<OStreamSink>(cap_child1, pattern);
        grandchild1->template add_sink<OStreamSink>(cap_grandchild1, pattern);
        grandchild2->template add_sink<OStreamSink>(cap_grandchild2, pattern);
        great_grandchild1->template add_sink<OStreamSink>(cap_great_grandchild1, pattern);
        great_grandchild2->template add_sink<OStreamSink>(cap_great_grandchild2, pattern);

        // Initially, test that child1 hierarchy works
        great_grandchild1->info(message);
        expect(
            cap_child1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_grandchild1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_great_grandchild1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(cap_grandchild2.read(), equal_to(String{}));
        expect(cap_great_grandchild2.read(), equal_to(String{}));
        expect(cap_sink1.read(), equal_to(String{}));
        expect(cap_sink2.read(), equal_to(String{}));

        // Also test grandchild2 propagates to child1
        grandchild2->info(message);
        expect(
            cap_child1.read(), equal_to(from_utf8<Char>("[grandchild2] ") + message + Char{'\n'}));
        expect(
            cap_grandchild2.read(),
            equal_to(from_utf8<Char>("[grandchild2] ") + message + Char{'\n'}));
        expect(cap_grandchild1.read(), equal_to(String{}));
        expect(cap_great_grandchild1.read(), equal_to(String{}));
        expect(cap_great_grandchild2.read(), equal_to(String{}));
        expect(cap_sink1.read(), equal_to(String{}));
        expect(cap_sink2.read(), equal_to(String{}));

        // Add sink1 to child1 - should propagate to its descendants but not to child2
        auto sink1 = child1->template add_sink<OStreamSink>(cap_sink1, pattern);

        // Test that great_grandchild1 now propagates to sink1
        great_grandchild1->info(message);
        expect(
            cap_child1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_grandchild1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_great_grandchild1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_sink1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(cap_sink2.read(), equal_to(String{}));

        // Test that great_grandchild2 also propagates to sink1
        great_grandchild2->info(message);
        expect(
            cap_child1.read(),
            equal_to(from_utf8<Char>("[great_grandchild2] ") + message + Char{'\n'}));
        expect(
            cap_grandchild1.read(),
            equal_to(from_utf8<Char>("[great_grandchild2] ") + message + Char{'\n'}));
        expect(
            cap_great_grandchild2.read(),
            equal_to(from_utf8<Char>("[great_grandchild2] ") + message + Char{'\n'}));
        expect(
            cap_sink1.read(),
            equal_to(from_utf8<Char>("[great_grandchild2] ") + message + Char{'\n'}));
        expect(cap_sink2.read(), equal_to(String{}));

        // Test that grandchild2 also propagates to sink1
        grandchild2->info(message);
        expect(
            cap_child1.read(), equal_to(from_utf8<Char>("[grandchild2] ") + message + Char{'\n'}));
        expect(
            cap_grandchild2.read(),
            equal_to(from_utf8<Char>("[grandchild2] ") + message + Char{'\n'}));
        expect(
            cap_sink1.read(), equal_to(from_utf8<Char>("[grandchild2] ") + message + Char{'\n'}));
        expect(cap_sink2.read(), equal_to(String{}));

        // Add sink2 to child2 - should NOT affect child1's descendants
        auto sink2 = child2->template add_sink<OStreamSink>(cap_sink2, pattern);

        // Verify child1 descendants still only go to sink1, not sink2
        great_grandchild1->info(message);
        expect(
            cap_child1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_grandchild1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_great_grandchild1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_sink1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(cap_sink2.read(), equal_to(String{}));

        // Test child2 itself would go to sink2 (if it logged)
        child2->info(message);
        expect(cap_sink1.read(), equal_to(String{}));
        expect(cap_sink2.read(), equal_to(from_utf8<Char>("[child2] ") + message + Char{'\n'}));

        // Disable sink1 in child1 - should stop propagation to it for all child1 descendants
        child1->set_sink_enabled(sink1, false);
        great_grandchild1->info(message);
        expect(
            cap_child1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_grandchild1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(
            cap_great_grandchild1.read(),
            equal_to(from_utf8<Char>("[great_grandchild1] ") + message + Char{'\n'}));
        expect(cap_sink1.read(), equal_to(String{}));
        expect(cap_sink2.read(), equal_to(String{}));

        // Remove sink1 completely - should stop propagation to it
        child1->remove_sink(sink1);
        grandchild2->info(message);
        expect(
            cap_child1.read(), equal_to(from_utf8<Char>("[grandchild2] ") + message + Char{'\n'}));
        expect(
            cap_grandchild2.read(),
            equal_to(from_utf8<Char>("[grandchild2] ") + message + Char{'\n'}));
        expect(cap_sink1.read(), equal_to(String{}));
        expect(cap_sink2.read(), equal_to(String{}));
    });
});

} // namespace
