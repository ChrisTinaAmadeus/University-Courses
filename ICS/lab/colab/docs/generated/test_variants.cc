// Quick test variants for h03 analysis
// Copy relevant sections into scheduler.cc to test

// Variant A: Disable steal, keep current wakeup
// Variant B: Source-only wakeup, keep current steal
// Variant C: Disable steal + source-only wakeup (match baseline decisions)
// Variant D: Conservative wakeup (only use idle if source queue > 1)
// Variant E: Conservative steal (only steal from large queues, >2 tasks)
