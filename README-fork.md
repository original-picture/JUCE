
# Parent/Child peers
This is a fork that adds support for parent-child relationships between `ComponentPeer`s
Adding a child peer generalize the behavior of owner/owned windows (windows/HWND), transient windows (X11/Window), and parent/child windows (macOS/NSWindow)


also to be clear, I'm not from JUCE! I'm just the person that made this fork

## Open question
* Should the parent/child peer related functions in `ComponentPeer` be forwarded and implemented in `Component` as well?

## Todo
- [x] Basic support for child windows on all platforms
  - [x] windows
  - [x] linux/X11
  - [x] macOS
- [ ] add additional helper functions in `Component` and add necessary plumbing to `Component` and `ComponentPeer`
  - [ ] add a list of child `ComponentPeer`s to `ComponentPeer`
- [ ] figure out how child windows should handle z-order (make sure they play nicely with functions like)
  - [ ] make child window z-order relative to transient siblings
    - [ ] `toBehind` does nothing if this peer is a child and `ComponentPeer other` isn't a sibling
      * or should I instead find the lowest common ancestor and then call `toBehind` on the children of the LCA that are ancestors of the two
        * sounds like a pain, I'll probably just not
- [ ] test for parent/child window cycles in `addChildPeer`?
  * `Component::addChildComponent` only tests to see if the child-to-be is equal to the parent-to-be and if the child-to-be is actually the parent of the parent-to-be, two special cases of cycles
  * could be done efficiently with floyd's two pointer algorithm
- [ ] figure out what should happen when calling `SetAlwaysOnTop(true)` on a peer that has children. Should `SetAlwaysOnTop(true)` be called recursively on the children?
  * I think the answer is yes
  - [ ] might need to define some kind of "hereditarily on top" concept (i.e., a window that is always on top but only because one of its ancestors is, and will cease to be on top if its on top ancestors are made not on top or are destroyed)
    - [x] afaik there is no `isAlwaysOnTop` public member variable or function in `Component` or `ComponentPeer`, but if one ever gets added, should it return whether the window is hereditarily on top or inherently on top?
     * I'd say it should return if either is true, but add an additional function called `isHereditarilyAlwaysOnTop`
- [ ] figure out what should happen when calling `SetAlwaysOnTop(true)` on a child peer when its parent isn't 
  * possibilites:
    1. call `SetAlwaysOnTop(true)` on all of the child's ancestors
       * I don't like this solution. It would be weird for an operation on a child to modify the child's parent
    2. unparent the child and then call `SetAlwaysOnTop(true)` on it
       * I think this is the way
       * question: does the child get reparented to its former parent if `SetAlwaysOnTop(false)` is called later?
         Or is their relationship terminated permanently
    3. or just let it be on top. I tested it on windows and yeah the behavior is kind of weird but if a user asks for that then they know what they're getting into
      * macos doesn't support this, so it's a no-go
- [ ] ~~don't allow an always on top window to add a not always on top window as a child. The reverse situation is okay though~~
  * no just promote the child window to be transiently always on top 
- [ ] also obviously early out of all functions that take two `ComponentPeer`s if the two peers are the same
- [ ] on windows, it seems that setting the owner window as `HWND_TOPMOST` will cause the owner window to stay over a non-topmost owned window. 
      in order to achieve the desired behavior of keeping child peers above their parents, regardless of whether the parent is always on top,
      it may be necessary for setAlwaysOnTop calls to propagate to all peers
- [X] Maybe check out `WS_EX_TOOLWINDOW` it might be useful
- [ ] add callbacks to `Component` for when various changes to the component's peer occur? (children added/removed, etc.)
- [ ] extract the array insertion from z-order code from `addChildComponent` and add it to a private static function template in `Component` that generically inserts into an array of any type
      so that the logic can be shared between `addChildComponent` and `addChildPeer`
  * do this for all complex operations that deal with component z-order (`toFront`, `toBack`, `toBehind`, etc.)
- [ ] ~~calling `toFront` on a child peer should call `toFront` on its parent too (and this continues recursively until a top level window is found)~~
  * actually no, I think calling `toFront` on a child window should only re-order windows relative to their siblings
  * or maybe add an optional `bool bringAncestorsToFrontToo` parameter
- [ ] when calling `addToDesktop` on a child component `C`, maybe get the peer of the component's former parent `P` and make `C` a child peer of `P`'s peer?
  * basically maintaining the parent/child relationship, just instead of being between components, now the relationship is between peers 
- [ ] uh oh. I just remembered that `addToDesktop` takes in a `nativeWindowToAttachTo`. 
      In the win32 implementation, `nativeWindowToAttachTo` is later referred to as parent window (which makes sense, because `nativeWindowToAttachTo` becomes a win32 parent window).
      `LinuxComponentPeer` has a `parentWindow` member too.
      This means that the parent/child nomenclature is already in use, so I need to find some other name for the hierarchical relationship I'm developing
  * Note: the parent/child terminology is only used for private variables and functions, so changing their names to be less ambiguous might be possible. Parents are referred to in public documentation comments though...
  * The whole `nativeWindowToAttachTo` system seems pretty "off the beaten path". Unlike something like child components, I don't think many people are using it (though I could be wrong), so maybe adding another idea (child peers) with a similar name won't be the end of the world, at least not for users of the library
    * and then again, it might even be possible to rename private variables and functions that refer to "parents" in "children", so this might not be an issue at all
  * I could call them owned windows like the windows api does. But I don't really like this term because I think using the term "owner" might make people who aren't familiar with win32 owned windows think that owner windows have something to do with C++ object ownership and lifetime, which is *not* the case
  * I could call them "top level parent/child windows", but that's still a little confusing
  * also it's important to note that all the underlying windowing systems have two distinct hierarchical systems for organizing windows (e.g. parent/child and owner/owned windows in win32)
    so I could sort of make an argument that this complexity is justified because it's just exposing complexity inherent to the underlying APIs, not inventing new complexity
    * but then again, if child peers were added, JUCE would then have **three** distinct systems hierarchical systems for organizing GUI elements (child components, `addToDesktop`/`nativeWindowToAttachTo`, and child peers)
      * buuuuuut on the other hand you could argue that it's not really that bad because, again, there probably aren't all that many people using `nativeWindowToAttachTo`, so it's more like 2.5 distinct systems
- [ ] make `addChildPeer` fail if the peer has already been attached to a native parent via `addToDesktop`?
- [ ] the `addToDesktop`/`nativeWindowToAttachTo` API seems kind of incomplete. Maybe give it some more utility functions
  * there are seemingly no functions for checking if a peer has a parent window, reparenting a window, enumerating child windows, etc. 
- [ ] when a child's parent is deleted, have it get adopted by its grandparent?
  * just mimic whatever component does
  * hide window when its parent dies?
  * also doesn't matter in practice because you really shouldn't close a window without closing its children
- [ ] make sure child windows receive minimization and close notifications
  * this might just work automatically 

- [ ] don't allow calling `toBehind` with an always on top window and a *not* always on top window
 * copy component's behavior 
- [ ] add ComponentPeer::toBack() 
  * use `SetWindowPos(..., HWND_BOTTOM)`, `XLowerWindow`, and `orderBack`
- [ ] making a parent window invisible makes its children invisible (recursively)

## Bugs
- [x] *indicates a fixed bug*  
  
- [x] the behavior of owned windows on win32 is slightly incorrect 
  * usually, a window that owns another windows always displays as a single window in the taskbar. 
    in juce, the owner window AND all the owned windows show up on the taskbar (and can be alt-tabbed into),
    UNLESS the owner window has been minimized, in which case only one window will display in the taskbar until the owner window is activated.
    Note that only minimization has this effect. Simply focusing another window doesn't change how the window(s) appear on the taskbar
  * this makes selecting a window from the taskbar slightly annoying, because two clicks are required to open an application that has owned windows
    (one click on the application icon, then another to select which window, even though clicking any window will have the same effect, because opening an owned window opens its owner window and vice versa)
  * this is a result of `WS_EX_APPWINDOW` being set. I could manually fix with `get/SetWindowLongPtr`, but I've noticed that juce also has an `appearsOnTaskbar` style flag, and I don't want to contradict it
    * ~~might be enough to just set the style flag. I don't see any style flag setters doing anything fancy~~
      * never mind, `styleFlags` is const so I'm just gonna not worry about it
  * **fixed by unsetting `WS_EX_APPWINDOW`**
- [ ] ~~minimizing the child window on macOS minimizes the parent window too.~~
  * ~~The parent window then can't be maximized by clicking its icon in the dock,
    the only way to bring it back is to click the icon of the minimized child window~~
  * ~~this might just be how child windows work on macOS~~
    * ~~could potentially be circumvented by unparenting the child window when its minimized and then reparenting it when it's maximized?~~ 
    * ~~idk seems convoluted and fragile~~
    * **apparently this behavior is simply unavoidable**
      * source: [this file from QT 4.8.6](https://github.com/Kitware/fletch/blob/70f4e025067453cbf2f40565c05d80c6263d64c8/Patches/Qt/4.8.6/gui/kernel/qwidget_mac.mm#L4) (ctrl+F for addChildWindow and read the relevant comment)

- [ ] not really even a bug, but moving the parent window on macOS moves the child window too
  * ~~this is seemingly just how things work on macOS~~
  * ~~not a huge deal but this behavior is inconsistent with how things work with win32 and X11~~
  * ~~tbh though might be a nonissue because maybe the differences between platforms will align with user expectations
    (e.g., Windows users expect an owned window to stay put when its owner is dragged and macOS users expect a child window to move with its parent)~~ 
  * **it seems this is unavoidable. See my note about minimizing child windows doing strange things on macOS**
- [ ] in my test code (not in this repo), child windows are always stacked in the order they were added as children.
      Clicking and dragging a window doesn't affect its z-order
    * irritatingly, `orderFront` and `orderBack` seemingly have no effect on child windows
    * I found a dumb hack though. If you remove the child window and then add it again it will show on top
      * maybe this could be exploited by overriding `windowDidBecomeKey` in `NSWindowDelegate`
    * handling more complex reordering operations like sending windows to the back or the middle would require removing and re-adding multiple (potentially all) child windows
    * Maybe this isn't even a bug. Is this just how things work on macOS? Is this the behavior macOS users would expect?
- [ ] on windows and macOS (but not X11/linux), deleting an owned window deletes all its child windows (the actual native windows get deleted, but the juce objects don't get deleted)
  * not shocking  considering the owner/owned nomenclature
  * maybe I should make the parent-child relationship owning on the C++ side too, 
    not just to fix this bug, but also because a child window really shouldn't outlive its parent.
    That would lead to a confusing user experience
  * no that's a bad idea. Just unparent child windows (at the native level) in the destructor

# Changes to existing parts of JUCE
* edited the comment of `ComponentPeer::setAlwaysOnTop` to remove language that referred to "siblings", 
  because with the addition of parent/child peers, the usage of that term could be confusing 
* 

# Notes
* what is `NSViewComponentPeer::wasAlwaysOnTop`?
* should an always on top window be on top of all other peers or just its siblings?
* sometimes JUCE internally destroys and recreates peers (see `Component::setAlwaysOnTop`). Will this break child peer functionality?

# Things to ask the maintainers
* I've added a protected member `internalIsInherentlyAlwaysOnTop` to `ComponentPeer`, 
  but `NSViewComponentPeer` already has a private member `isAlwaysOnTop`. 
  Having two similar members is kind of confusing. 
  Can we refactor `NSViewComponentPeer` so that it uses the member variable it inherits from `ComponentPeer`?
