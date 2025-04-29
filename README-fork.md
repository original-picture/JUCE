
# Parent/Child peers
This is a fork that adds support for parent-child relationships between `ComponentPeer`s
Adding a child peer generalize the behavior of owner/owned windows (windows/HWND), transient windows (X11/Window), and parent/child windows (macOS/NSWindow)


also to be clear, I'm not from JUCE! I'm just the person that made this fork

## Open question
* Should the parent/child peer related functions in `ComponentPeer` be forwarded and implemented in `Component` as well?

## Todo
- [ ] Basic support for transient windows on all platforms
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
- [ ] automatically call `setAlwaysOnTop(false)` when adding a transient child 
- [ ] figure out what should happen when calling `SetAlwaysOnTop(true)` on a peer that has children. Should the children return 
- [ ] don't allow an always on top window to add a not always on top window as a child. The reverse situation is okay though
- [ ] also obviously early out of all functions that take two `ComponentPeer`s if the two peers are the same
- [ ] on windows, it seems that setting the owner window as `HWND_TOPMOST` will cause the owner window to stay over a non-topmost owned window. 
      in order to achieve the desired behavior of keeping child peers above their parents, regardless of whether the parent is always on top,
      it may be necessary for setAlwaysOnTop calls to propagate to all peers
- [ ] Maybe check out `WS_EX_TOOLWINDOW` it might be useful

## Bugs
- [x] *indicates a fixed bug*  
  
- [ ] the behavior of owned windows on win32 is slightly incorrect
  * usually, a window that owns another windows always displays as a single window in the taskbar. 
    in juce, the owner window AND all the owned windows show up on the taskbar (and can be alt-tabbed into),
    UNLESS the owner window has been minimized, in which case only one window will display in the taskbar until the owner window is activated.
    Note that only minimization has this effect. Simply focusing another window doesn't change how the window(s) appear on the taskbar
  * this makes selecting a window from the taskbar slightly annoying, because two clicks are required to open an application that has owned windows
    (one click on the application icon, then another to select which window, even though clicking any window will have the same effect, because opening an owned window opens its owner window and vice versa)
