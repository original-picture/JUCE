
# Transient windows
This is a fork that adds support for what I'm calling *transient windows*, which generalize the behavior of owner/owned windows (win32), transient windows (X11), and parent/child windows (macOS)
I chose the term transient window because parent/child and owner/owned are pretty overloaded terms. In comparison, transient is less ambiguous

also to be clear, I'm not from JUCE! I'm just the person that made this fork

## Todo
- [ ] Basic support for transient windows on all platform
  - [x] windows
  - [x] linux/X11
  - [ ] macOS
- [ ] modify existing JUCE functionality to reflect the fact that transient owner/owned relationships and JUCE parent/child relationships are mutually exclusive
  - [ ] terminate any existing parent/child relationship when making a window transient for another window
  - [ ] terminate any existing transient for relationship when making a window the child of another window
- [ ] add additional helper functions in `Component` and add necessary plumbing to `Component` and `ComponentPeer`
  - [ ] add a list of owned `ComponentPeer`s to `ComponentPeer`
- [ ] figure out how transient windows should handle z-order (make sure they play nicely with functions like)
  - [ ] make transient window z-order relative to transient siblings
    - [ ] `toBehind` does nothing if this peer is transient and `ComponentPeer other` isn't a sibling
- [ ] test for transient window cycles?
- [ ] maybe rename `setTransientFor` to `makeTransientFor`