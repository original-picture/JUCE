/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

//==============================================================================
/**
    The Component class uses a ComponentPeer internally to create and manage a real
    operating-system window.

    This is an abstract base class - the platform specific code contains implementations of
    it for the various platforms.

    User-code should very rarely need to have any involvement with this class.

    @see Component::createNewPeer

    @tags{GUI}
*/
class JUCE_API  ComponentPeer : private FocusChangeListener
{
public:
    //==============================================================================
    /** A combination of these flags is passed to the ComponentPeer constructor. */
    enum StyleFlags
    {
        windowAppearsOnTaskbar                          = (1 << 0),   /**< Indicates that the window should have a corresponding
                                                                           entry on the taskbar (ignored on MacOSX) */
        windowIsTemporary                               = (1 << 1),   /**< Indicates that the window is a temporary popup, like a menu,
                                                                           tooltip, etc. */
        windowIgnoresMouseClicks                        = (1 << 2),   /**< Indicates that the window should let mouse clicks pass
                                                                           through it (may not be possible on some platforms). */
        windowHasTitleBar                               = (1 << 3),   /**< Indicates that the window should have a normal OS-specific
                                                                           title bar and frame. if not specified, the window will be
                                                                           borderless. */
        windowIsResizable                               = (1 << 4),   /**< Indicates that the window should have a resizable border. */
        windowHasMinimiseButton                         = (1 << 5),   /**< Indicates that if the window has a title bar, it should have a
                                                                           minimise button on it. */
        windowHasMaximiseButton                         = (1 << 6),   /**< Indicates that if the window has a title bar, it should have a
                                                                           maximise button on it. */
        windowHasCloseButton                            = (1 << 7),   /**< Indicates that if the window has a title bar, it should have a
                                                                           close button on it. */
        windowHasDropShadow                             = (1 << 8),   /**< Indicates that the window should have a drop-shadow (this may
                                                                           not be possible on all platforms). */
        windowRepaintedExplicitly                       = (1 << 9),   /**< Not intended for public use - this tells a window not to
                                                                           do its own repainting, but only to repaint when the
                                                                           performAnyPendingRepaintsNow() method is called. */
        windowIgnoresKeyPresses                         = (1 << 10),  /**< Tells the window not to catch any keypresses. This can
                                                                           be used for things like plugin windows, to stop them interfering
                                                                           with the host's shortcut keys. */
        windowRequiresSynchronousCoreGraphicsRendering  = (1 << 11),  /**< Indicates that the window should not be rendered with
                                                                           asynchronous Core Graphics drawing operations. Use this if there
                                                                           are issues with regions not being redrawn at the expected time
                                                                           (macOS and iOS only). */
        windowIsSemiTransparent                         = (1 << 30),  /**< Not intended for public use - makes a window transparent. */

        windowUsesNormalTitlebarWhenSkippingTaskbar      = (1 << 31)   /**< Only used on windows.
                                                                           If this flag is used and windowAppearsOnTaskbar is not set, the WS_EX_TOOLWINDOW style will not be used.
                                                                           Instead, WS_EX_APPWINDOW will be omitted, causing the window to skip the taskbar while still having the usual window decorations.
                                                                           Ignored if windowAppearsOnTaskbar is set.
                                                                           Note that windows seems to force the first created window onto the toolbar if it doesn't have the WS_EX_TOOLWINDOW style, even if WS_EX_APPWINDOW is not specified.
                                                                           This is usually not an issue in practice, because windowAppearsOnTaskbar is typically only unset for secondary tool windows,
                                                                           while the main window of the application almost always *does* include windowAppearsOnTaskbar.
                                                                           So only use this flag if you have some primary application window that does show on the taskbar,
                                                                           and even in that case, only use this flag for secondary windows that get created after the main window */

    };

    /** Represents the window borders around a window component.

        You must use operator bool() to evaluate the validity of the object before accessing
        its value.

        Returned by getFrameSizeIfPresent(). A missing value may be returned on Linux for a
        short time after window creation.
    */
    class JUCE_API  OptionalBorderSize final
    {
    public:
        /** Default constructor. Creates an invalid object. */
        OptionalBorderSize()                               : valid (false)                               {}

        /** Constructor. Creates a valid object containing the provided BorderSize<int>. */
        explicit OptionalBorderSize (BorderSize<int> size) : valid (true), borderSize (std::move (size)) {}

        /** Returns true if a valid value has been provided. */
        explicit operator bool() const noexcept { return valid; }

        /** Returns a reference to the value.

            You must not call this function on an invalid object. Use operator bool() to
            determine validity.
        */
        const auto& operator*() const noexcept
        {
            jassert (valid);
            return borderSize;
        }

        /** Returns a pointer to the value.

            You must not call this function on an invalid object. Use operator bool() to
            determine validity.
        */
        const auto* operator->() const noexcept
        {
            jassert (valid);
            return &borderSize;
        }

    private:
        bool valid;
        BorderSize<int> borderSize;
    };

    //==============================================================================
    /** Creates a peer.

        The component is the one that we intend to represent, and the style flags are
        a combination of the values in the StyleFlags enum
    */
    ComponentPeer (Component& component, int styleFlags);

    /** Destructor. */
    ~ComponentPeer() override;

    //==============================================================================
    /** Returns the component being represented by this peer. */
    Component& getComponent() noexcept                      { return component; }

    /** Returns the set of style flags that were set when the window was created.
        @see Component::addToDesktop
    */
    int getStyleFlags() const noexcept                      { return styleFlags; }

    /** Returns a unique ID for this peer.
        Each peer that is created is given a different ID.
    */
    uint32 getUniqueID() const noexcept                     { return uniqueID; }

    //==============================================================================
    /** Returns the raw handle to whatever kind of window is being used.

        On windows, this is probably a HWND, on the mac, it's likely to be a WindowRef,
        but remember there's no guarantees what you'll get back.
    */
    virtual void* getNativeHandle() const = 0;

    /** Shows or hides the window. */
    void setVisible (bool shouldBeVisible);

    /** Changes the title of the window. */
    virtual void setTitle (const String& title) = 0;

    /** If this type of window is capable of indicating that the document in it has been
        edited, then this changes its status.

        For example in OSX, this changes the appearance of the close button.
        @returns true if the window has a mechanism for showing this, or false if not.
    */
    virtual bool setDocumentEditedStatus (bool edited);

    /** If this type of window is capable of indicating that it represents a file, then
        this lets you set the file.

        E.g. in OSX it'll show an icon for the file in the title bar.
    */
    virtual void setRepresentedFile (const File&);

    //==============================================================================
    /** Moves and resizes the window.

        If the native window is contained in another window, then the coordinates are
        relative to the parent window's origin, not the screen origin.

        This should result in a callback to handleMovedOrResized().
    */
    virtual void setBounds (const Rectangle<int>& newBounds, bool isNowFullScreen) = 0;

    /** Updates the peer's bounds to match its component. */
    void updateBounds();

    /** Returns the current position and size of the window.

        If the native window is contained in another window, then the coordinates are
        relative to the parent window's origin, not the screen origin.
    */
    virtual Rectangle<int> getBounds() const = 0;

    /** Converts a position relative to the top-left of this component to screen coordinates. */
    virtual Point<float> localToGlobal (Point<float> relativePosition) = 0;

    /** Converts a screen coordinate to a position relative to the top-left of this component. */
    virtual Point<float> globalToLocal (Point<float> screenPosition) = 0;

    /** Converts a position relative to the top-left of this component to screen coordinates. */
    Point<int> localToGlobal (Point<int> relativePosition);

    /** Converts a screen coordinate to a position relative to the top-left of this component. */
    Point<int> globalToLocal (Point<int> screenPosition);

    /** Converts a rectangle relative to the top-left of this component to screen coordinates. */
    virtual Rectangle<int> localToGlobal (const Rectangle<int>& relativePosition);

    /** Converts a screen area to a position relative to the top-left of this component. */
    virtual Rectangle<int> globalToLocal (const Rectangle<int>& screenPosition);

    /** Converts a rectangle relative to the top-left of this component to screen coordinates. */
    Rectangle<float> localToGlobal (const Rectangle<float>& relativePosition);

    /** Converts a screen area to a position relative to the top-left of this component. */
    Rectangle<float> globalToLocal (const Rectangle<float>& screenPosition);

    /** Returns the area in peer coordinates that is covered by the given sub-comp (which
        may be at any depth)
    */
    Rectangle<int> getAreaCoveredBy (const Component& subComponent) const;


    /** Sets this window to either be always-on-top or normal.
        Some kinds of window might not be able to do this, so should return false.
        If this peer has any floating children, they will become always on top too,
        and will lose their always on top status when this window loses its always on top status
        (unless setAlwaysOnTop (true) is called on the children as well).
        The always on top status of ancestor peers is unaffected
    */
    bool setAlwaysOnTop (bool alwaysOnTop);

    /** Returns true if this peer is set to always stay in front of other windows on the desktop.
        Equivalent to isInherentlyAlwaysOnTop() || hasInherentlyAlwaysOnTopAncestor()
        @see setAlwaysOnTop, hasInherentlyAlwaysOnTopAncestor, isInherentlyAlwaysOnTop
    */
    bool isAlwaysOnTop() const noexcept;

    /** Returns true if this component is always on top because setAlwaysOnTop(true) was called on it specifically.
        Note that hasInherentlyAlwaysOnTopAncestor() and isInherentlyAlwaysOnTop() are not mutually exclusive!
        @see setAlwaysOnTop, isAlwaysOnTop, hasInherentlyAlwaysOnTopAncestor
    */
    bool isInherentlyAlwaysOnTop() const noexcept;

    /** Returns true if this peer has ancestors that are always on top.
        Note that hasInherentlyAlwaysOnTopAncestor() and isInherentlyAlwaysOnTop() are not mutually exclusive!
        @see setAlwaysOnTop, isAlwaysOnTop, isInherentlyAlwaysOnTop
    */
    bool hasInherentlyAlwaysOnTopAncestor() const noexcept;

    bool isInherentlyAlwaysOnTopOrHasInherentlyAlwaysOnTopAncestor();


    /** Minimises the window. Child peers will be minimized recursively. */
    void setMinimised (bool shouldBeMinimised);

    /** Returns true if this peer is minimised.
        @see setMinimised
    */
    virtual bool isMinimised() const = 0;

    /** Returns true if this peer is minimised because setMinimised (true) was called on it specifically.
        On some platforms, isMinimised might return true for windows that have a minimised ancestor,
        so this function can be used to differentiate between inherently and ancestralyl minimised windows
    */
    bool isInherentlyMinimised() const noexcept;

    /** True if this peer has one or more inherently minimised ancestors. */
    bool hasInherentlyMinimisedAncestor() const noexcept;

    /** Returns true if this peer is inherently minimised OR has at least one inherently minimised ancestor (or ancestors). */
    bool isInherentlyMinimisedOrHasInherentlyMinimisedAncestor() const noexcept;


    /** Returns true if this component is hidden because setHidden (true) was called on it specifically.
        Useful because isShowing doesn't give you enough information about *why* a peer is/isn't showing.
        (a peer being inherently hidden, having an inherently hidden ancestor, and/or having a minimised ancestor can all cause isShowing to return false)
        @see setVisible, isShowing, isInherentlyHidden, hasInherentlyHiddenAncestor, isInherentlyHiddenOrHasInherentlyHiddenAncestor
    */
    bool isInherentlyHidden() const noexcept;

    /** True if this peer has one or more inherently hidden ancestors. */
    bool hasInherentlyHiddenAncestor() const noexcept;

    /** Returns true if this peer is inherently hidden OR has at least one inherently hidden ancestor (or ancestors). */
    bool isInherentlyHiddenOrHasInherentlyHiddenAncestor() const noexcept;

    /** True if the window is being displayed on-screen. */
    virtual bool isShowing() const = 0;

    /** Enable/disable fullscreen mode for the window. */
    virtual void setFullScreen (bool shouldBeFullScreen) = 0;

    /** True if the window is currently full-screen. */
    virtual bool isFullScreen() const = 0;

    /** True if the window is in kiosk-mode. */
    virtual bool isKioskMode() const;

    /** Sets the size to restore to if fullscreen mode is turned off. */
    void setNonFullScreenBounds (const Rectangle<int>& newBounds) noexcept;

    /** Returns the size to restore to if fullscreen mode is turned off. */
    const Rectangle<int>& getNonFullScreenBounds() const noexcept;

    /** Attempts to change the icon associated with this window. */
    virtual void setIcon (const Image& newIcon) = 0;

    /** Sets a constrainer to use if the peer can resize itself.
        The constrainer won't be deleted by this object, so the caller must manage its lifetime.
    */
    void setConstrainer (ComponentBoundsConstrainer* newConstrainer) noexcept;

    /** Asks the window-manager to begin resizing this window, on platforms where this is useful
        (currently just Linux/X11).

        @param mouseDownPosition    The position of the mouse event that started the resize in
                                    unscaled peer coordinates
        @param zone                 The edges of the window that may be moved during the resize
    */
    virtual void startHostManagedResize ([[maybe_unused]] Point<int> mouseDownPosition,
                                         [[maybe_unused]] ResizableBorderComponent::Zone zone) {}

    /** Returns the current constrainer, if one has been set. */
    ComponentBoundsConstrainer* getConstrainer() const noexcept             { return constrainer; }

    /** Checks if a point is in the window.

        The position is relative to the top-left of this window, in unscaled peer coordinates.
        If trueIfInAChildWindow is false, then this returns false if the point is actually
        inside a child of this window.
    */
    virtual bool contains (Point<int> localPos, bool trueIfInAChildWindow) const = 0;

    /** Returns the size of the window frame that's around this window.

        Depending on the platform the border size may be invalid for a short transient
        after creating a new window. Hence the returned value must be checked using
        operator bool() and the contained value can be accessed using operator*() only
        if it is present.

        Whether or not the window has a normal window frame depends on the flags
        that were set when the window was created by Component::addToDesktop()
    */
    virtual OptionalBorderSize getFrameSizeIfPresent() const = 0;

    /** Returns the size of the window frame that's around this window.
        Whether or not the window has a normal window frame depends on the flags
        that were set when the window was created by Component::addToDesktop()
    */
   #if JUCE_LINUX || JUCE_BSD
    [[deprecated ("Use getFrameSizeIfPresent instead.")]]
   #endif
    virtual BorderSize<int> getFrameSize() const = 0;

    /** This is called when the window's bounds change.
        A peer implementation must call this when the window is moved and resized, so that
        this method can pass the message on to the component.
    */
    void handleMovedOrResized();

    /** This is called if the screen resolution changes.
        A peer implementation must call this if the monitor arrangement changes or the available
        screen size changes.
    */
    virtual void handleScreenSizeChange();

    //==============================================================================
    /** This is called to repaint the component into the given context.

        Increments the result of getNumFramesPainted().
    */
    void handlePaint (LowLevelGraphicsContext& contextToPaintTo);

    //==============================================================================
    /** Returns the number of top level child peers that this peer is a parent of. */
    int getNumFloatingChildPeers() const noexcept;

    /** Returns one of this peer's floating child peers, by it index.

        The peer with index 0 is at the back of the z-order, the one at the
        front will have index (getNumFloatingChildPeers() - 1).

        If the index is out-of-range, this will return a null pointer.
    */
    ComponentPeer* getFloatingChildPeer (int index) const noexcept;

    /** Returns the index of this peer in the list of floating child peers.

        A value of 0 means it is first in the list (i.e. behind all other peers). Higher
        values are further towards the front.

        Returns -1 if the component passed-in is not a child of this component.

        @see getChildren, getNumChildComponents, getChildComponent, addChildComponent, toFront, toBack, toBehind
    */
    int getIndexOfFloatingChildPeer (const ComponentPeer* child) const noexcept;

    /** Returns a reference to the underlying array of child peers. */
    const Array<ComponentPeer*>& getFloatingChildPeers() const noexcept { return floatingChildPeerList; }

    /** Add a floating child peer to this peer.

        A floating child peer will always stay on top of its parent, but not other peers.
        Floating child peers are useful for implementing secondary tool windows for your application or plugin.

        Note that a parent peer does not own its floating children! Children do not get destroyed when their parent is destroyed.

        Also note that the hierarchical relationship created by the nativeWindowToAttachTo parameter of Component::addToDesktop
        is different from the hierarchical relationship created by addFloatingChildPeer!
        Long story short, nativeWindowToAttachTo and addFloatingChildPeer map to different systems of the underlying OS-specific APIs.
        For example, specifying nativeWindowToAttachTo creates a win32 *child* window, while addFloatingChildPeer creates a win32 *owned* window. MacOS and linux have analogous concepts.
        These two systems are mutually exclusive. So don't try to add a peer as floating child if it's already been added attached to a parent with addToDesktop.

        @param child     the child peer to add. If the peer passed-in is already
                         the child of another peer, it'll first be removed from its current parent.
        @param zOrder    The index in the child-list at which this peer should be inserted.
                         A value of -1 will insert it in front of the others, 0 is the back.
        @return          False if the child couldn't be added (e.g., because the platform doesn't support floating child peers)
    */
    bool addFloatingChildPeer (ComponentPeer* child, int zOrder = -1);

    /** Returns the floating child peer with the given ID,
        or nullptr, if this peer doesn't have a child with the given ID
    */
    ComponentPeer* findFloatingChildPeerWithID (uint32 targetID) const noexcept;

    /** Add a floating child peer to this peer.

        A floating child peer will always stay on top of its parent, but not other peers.
        Floating child peers are useful for implementing secondary tool windows for your application or plugin.

        Note that a parent peer does not own its floating children! Children do not get destroyed when their parent is destroyed.

        Also note that the hierarchical relationship created by the nativeWindowToAttachTo parameter of Component::addToDesktop
        is different from the hierarchical relationship created by addFloatingChildPeer!
        Long story short, nativeWindowToAttachTo and addFloatingChildPeer map to different systems of the underlying OS-specific APIs.
        For example, specifying nativeWindowToAttachTo creates a win32 *child* window, while addFloatingChildPeer creates a win32 *owned* window. MacOS and linux have analogous concepts.
        These two systems are mutually exclusive. So don't try to add a peer as floating child if it's already been added attached to a parent with addToDesktop.

        @param child     the child peer to add. If the peer passed-in is already
                         the child of another peer, it'll first be removed from its current parent.
        @param zOrder    The index in the child-list at which this peer should be inserted.
                         A value of -1 will insert it in front of the others, 0 is the back.
        @return          False if the child couldn't be added (e.g., because the platform doesn't support floating child peers)
    */
    bool addFloatingChildPeer (ComponentPeer& child, int zOrder = -1);

    /** Removes one of this peer's child peers.

        If the child passed-in isn't actually a child of this peer (either because
        it's invalid or is the child of a different parent), then no action is taken.

        Note that removing a child will not delete it! But it's ok to delete a component
        without first removing it - doing so will automatically remove it and send out the
        appropriate notifications before the deletion completes.

        @see addFloatingChildPeer, ComponentListener::componentChildrenChanged
    */
    void removeFloatingChildPeer (ComponentPeer* childToRemove);

    ComponentPeer* removeFloatingChildPeer (int childIndexToRemove);

    void removeAllFloatingChildren();

    ComponentPeer* getFloatingChildPeerParent() const noexcept;

    /** Returns the ancestor of this peer that has no parent, or this peer, if this peer itself has no parent. */
    ComponentPeer* getTopLevelPeer() noexcept;

    /** True if possibleDescendant appears anywhere in this peer's hierarchy.
         Does not return true if this and possibleDescendant refer to the same peer. */
    bool isAncestorOf (ComponentPeer* possibleDescendant) const noexcept;

    /** Brings the window to the top, optionally also giving it keyboard focus. */
    virtual void toFront (bool takeKeyboardFocus) = 0;

    /** Moves the window to be just behind another one.
        If this peer and other are cousins, not direct siblings, then the lowest common ancestor of the two peers is found,
        and the child of the LCA that is an ancestor of this peer is moved behind the child of the LCA that is an ancestor of other.
        Does nothing if this peer is an ancestor of other, or if other is an ancestor of this.
    */
    void toBehind (ComponentPeer* other);

    /** Called when the window is brought to the front, either by the OS or by a call
        to toFront().
    */
    void handleBroughtToFront();

    //==============================================================================
    /** True if the window has the keyboard focus. */
    virtual bool isFocused() const = 0;

    /** Tries to give the window keyboard focus. */
    virtual void grabFocus() = 0;

    /** Called when the window gains keyboard focus. */
    void handleFocusGain();
    /** Called when the window loses keyboard focus. */
    void handleFocusLoss();

    Component* getLastFocusedSubcomponent() const noexcept;

    /** Called when a key is pressed.
        For keycode info, see the KeyPress class.
        Returns true if the keystroke was used.
    */
    bool handleKeyPress (int keyCode, juce_wchar textCharacter);

    /** Called when a key is pressed.
        Returns true if the keystroke was used.
    */
    bool handleKeyPress (const KeyPress& key);

    /** Called whenever a key is pressed or released.
        Returns true if the keystroke was used.
    */
    bool handleKeyUpOrDown (bool isKeyDown);

    /** Called whenever a modifier key is pressed or released. */
    void handleModifierKeysChange();

    /** If there's a currently active input-method context - i.e. characters are being
        composed using multiple keystrokes - this should commit the current state of the
        context to the text and clear the context. This should not hide the virtual keyboard.
    */
    virtual void closeInputMethodContext();

    /** Alerts the peer that the current text input target has changed somehow.

        The peer may hide or show the virtual keyboard as a result of this call.
    */
    void refreshTextInputTarget();

    //==============================================================================
    /** Returns the currently focused TextInputTarget, or null if none is found. */
    TextInputTarget* findCurrentTextInputTarget();

    //==============================================================================
    /** Invalidates a region of the window to be repainted asynchronously. */
    virtual void repaint (const Rectangle<int>& area) = 0;

    /** This can be called (from the message thread) to cause the immediate redrawing
        of any areas of this window that need repainting.

        You shouldn't ever really need to use this, it's mainly for special purposes
        like supporting audio plugins where the host's event loop is out of our control.
    */
    virtual void performAnyPendingRepaintsNow() = 0;

    /** Changes the window's transparency. */
    virtual void setAlpha (float newAlpha) = 0;

    //==============================================================================
    void handleMouseEvent (MouseInputSource::InputSourceType type, Point<float> positionWithinPeer, ModifierKeys newMods, float pressure,
                           float orientation, int64 time, PenDetails pen = {}, int touchIndex = 0);

    void handleMouseWheel (MouseInputSource::InputSourceType type, Point<float> positionWithinPeer,
                           int64 time, const MouseWheelDetails&, int touchIndex = 0);

    void handleMagnifyGesture (MouseInputSource::InputSourceType type, Point<float> positionWithinPeer,
                               int64 time, float scaleFactor, int touchIndex = 0);

    void handleUserClosingWindow();

    /** Structure to describe drag and drop information */
    struct DragInfo
    {
        StringArray files;
        String text;
        Point<int> position;

        bool isEmpty() const noexcept       { return files.size() == 0 && text.isEmpty(); }
        void clear() noexcept               { files.clear(); text.clear(); }
    };

    bool handleDragMove (const DragInfo&);
    bool handleDragExit (const DragInfo&);
    bool handleDragDrop (const DragInfo&);

    //==============================================================================
    /** Returns the number of currently-active peers.
        @see getPeer
    */
    static int getNumPeers() noexcept;

    /** Returns one of the currently-active peers.
        @see getNumPeers
    */
    static ComponentPeer* getPeer (int index) noexcept;

    /** Returns the peer that's attached to the given component, or nullptr if there isn't one. */
    static ComponentPeer* getPeerFor (const Component*) noexcept;

    /** Checks if this peer object is valid.
        @see getNumPeers
    */
    static bool isValidPeer (const ComponentPeer* peer) noexcept;

    //==============================================================================
    virtual StringArray getAvailableRenderingEngines() = 0;
    virtual int getCurrentRenderingEngine() const;
    virtual void setCurrentRenderingEngine (int index);

    //==============================================================================
    /** On desktop platforms this method will check all the mouse and key states and return
        a ModifierKeys object representing them.

        This isn't recommended and is only needed in special circumstances for up-to-date
        modifier information at times when the app's event loop isn't running normally.

        Another reason to avoid this method is that it's not stateless and calling it may
        update the ModifierKeys::currentModifiers object, which could cause subtle changes
        in the behaviour of some components.
    */
    static ModifierKeys getCurrentModifiersRealtime() noexcept;

    //==============================================================================
    /**  Used to receive callbacks when the OS scale factor of this ComponentPeer changes.

         This is used internally by some native JUCE windows on Windows and Linux and you
         shouldn't need to worry about it in your own code unless you are dealing directly
         with native windows.
    */
    struct JUCE_API  ScaleFactorListener
    {
        /** Destructor. */
        virtual ~ScaleFactorListener() = default;

        /** Called when the scale factor changes. */
        virtual void nativeScaleFactorChanged (double newScaleFactor) = 0;
    };

    /** Adds a scale factor listener. */
    void addScaleFactorListener (ScaleFactorListener* listenerToAdd)          { scaleFactorListeners.add (listenerToAdd); }

    /** Removes a scale factor listener. */
    void removeScaleFactorListener (ScaleFactorListener* listenerToRemove)    { scaleFactorListeners.remove (listenerToRemove);  }

    //==============================================================================
    /** Used to receive callbacks on every vertical blank event of the display that the peer
        currently belongs to.

        On Linux this is currently limited to receiving callbacks from a timer approximately at
        display refresh rate.

        This is a low-level facility used by the peer implementations. If you wish to synchronise
        Component events with the display refresh, you should probably use the VBlankAttachment,
        which automatically takes care of listening to the vblank events of the right peer.

        @see VBlankAttachment
    */
    struct JUCE_API  VBlankListener
    {
        /** Destructor. */
        virtual ~VBlankListener() = default;

        /** Called on every vertical blank of the display to which the peer is associated.

            The timestampSec parameter is a monotonically increasing value expressed in seconds
            that corresponds to the time at which the next frame will be displayed.
        */
        virtual void onVBlank (double timestampSec) = 0;
    };

    /** Adds a VBlankListener. */
    void addVBlankListener (VBlankListener* listenerToAdd)       { vBlankListeners.add (listenerToAdd); }

    /** Removes a VBlankListener. */
    void removeVBlankListener (VBlankListener* listenerToRemove) { vBlankListeners.remove (listenerToRemove); }

    //==============================================================================
    /** On Windows and Linux this will return the OS scaling factor currently being applied
        to the native window. This is used to convert between physical and logical pixels
        at the OS API level and you shouldn't need to use it in your own code unless you
        are dealing directly with the native window.
    */
    virtual double getPlatformScaleFactor() const noexcept    { return 1.0; }

    /** On platforms that support it, this will update the window's titlebar in some
        way to indicate that the window's document needs saving.
    */
    virtual void setHasChangedSinceSaved (bool) {}


    enum class Style
    {
        /** A style that matches the system-wide style. */
        automatic,

        /** A light style, which will probably use dark text on a light background. */
        light,

        /** A dark style, which will probably use light text on a dark background. */
        dark
    };

    /** On operating systems that support it, this will update the style of this
        peer as requested.

        Note that this will not update the theme system-wide. This will only
        update UI elements so that they display appropriately for this peer!
    */
    void setAppStyle (Style s)
    {
        if (std::exchange (style, s) != style)
            appStyleChanged();
    }

    /** Returns the style requested for this app. */
    Style getAppStyle() const { return style; }

    /** Returns the number of times that this peer has been painted.

        This is mainly useful when debugging component painting. For example, you might use this to
        match logging calls to specific frames.
    */
    uint64_t getNumFramesPainted() const { return peerFrameNumber; }

protected:
    //==============================================================================
    static void forceDisplayUpdate();
    void callVBlankListeners (double timestampSec);

    /** This is what actually calls the platform specific code (SetWindowLongPtr, XSetTransientFor, addChildWindow) that creates the parent/child window relationship.*/
    virtual void setFloatingChildPeerNativeParent (ComponentPeer* parent) = 0;

    /** Undoes a native parent/child relationship created by setFloatingChildPeerNativeParent. */
    virtual void clearFloatingChildPeerNativeParent() = 0;

    /** called in the destructors of derived classes (HWNDComponentPeer and friends)
        we can't call it directly from ~ComponentPeer because we need to remove window from its parent before the platform specific destroy function is called,
        and by the time we reach ~ComponentPeer it's already too late
        also it calls pure virtual functions and obviously those can't be called from the destructor of the base class
    */
    void doFloatingChildPeerCleanup();

    /** Make callback return true in order to early out.
        Alternatively, it can return void if you don't need to early out.
    */
    template <typename Callback>
    bool forEachFloatingChildPeerAncestorPeerFromThisToRoot (Callback&& callback)
    {
        auto currentPeer = this;

        do
        {
            if constexpr (std::is_convertible_v<decltype(callback(currentPeer)), bool>)
            {
                if (callback (currentPeer))
                    return true;
            }
            else
                callback (currentPeer);
        }
        while ((currentPeer = currentPeer->floatingChildPeerParent) != nullptr);

        return false;
    }

    /** Make callback return true in order to early out.
        Alternatively, it can return void if you don't need to early out.
    */
    template <typename Callback>
    bool forEachFloatingChildPeerAncestorPeerFromRootToThis (Callback&& callback, bool processThisPointer = true)
    {
        Array<ComponentPeer*> peersToProcess; // use Array as a stack because I don't want to have to include <stack>

        { // limit the scope of peer
            ComponentPeer* peer = (processThisPointer ? this : this->floatingChildPeerParent);
            while (peer != nullptr)
            {
                peersToProcess.add (peer); // push this and all of its ancestors onto the stack
                peer = peer->floatingChildPeerParent;
            }
        }

        while (! peersToProcess.isEmpty()) // then pop each one off the stack and invoke callback on it
        {
            auto* peer = peersToProcess.getLast();
            peersToProcess.removeLast();

            if constexpr (std::is_convertible_v<decltype(callback (peer)), bool>)
            {
                if (callback (peer))
                    return true;
            }
            else
                callback (peer);
        }

        return false;
    }

    void makeAllAncestorsVisibleAndNotMinimised();

    virtual void setMinimisedWithoutSettingFlag (bool shouldBeMinimised) = 0;
    void setMinimisedRecursivelyWithoutSettingFlag (bool shouldBeMinimised);

    virtual void setVisibleWithoutSettingFlag (bool shouldBeVisible) = 0;
    void setVisibleRecursivelyWithoutSettingFlag (bool shouldBeVisible);

    virtual void toBehindInternal (ComponentPeer* other) = 0;
    void callToBehindInternalAndRearrangeChildList (ComponentPeer* other);

    Component& component;
    const int styleFlags;
    Rectangle<int> lastNonFullscreenBounds;
    ComponentBoundsConstrainer* constrainer = nullptr;
    static std::function<ModifierKeys()> getNativeRealtimeModifiers;
    ListenerList<ScaleFactorListener> scaleFactorListeners;
    ListenerList<VBlankListener> vBlankListeners;
    Style style = Style::automatic;

    /** True if this peer is attached to another window via the nativeWindowToAttachTo parameter of Component::addToDesktop.
        See the comment on ComponentPeer::addFloatingChildPeer for more details on nativeWindowToAttachTo and its relationship to floating child peers.
    */
    virtual bool isAttachedToAnotherWindow() = 0;

    /** If this peer is a floating child peer, then this member variable is equal to its parent.
        I chose this slightly weird named (instead of just "parent"), because some of the ComponentPeer implementation classes (HWNDComponentPeer, etc.)
        Have a member called something like "parent" that is used to keep track of the value passed into the nativeWindowToAttachTo parameter of Component::addToDesktop.
        I wanted to make it really clear that this is something different
    */
    ComponentPeer* floatingChildPeerParent = nullptr;
    Array<ComponentPeer*> floatingChildPeerList;
    bool internalIsInherentlyAlwaysOnTop = false;
    bool internalIsInherentlyMinimised   = false;
    bool internalIsInherentlyHidden      = false;

    /** Part of a very nasty workaround used in the win32 implementation
        Basically certain operations on windows can cause a sort of chain reaction of events in the windowProc
        that will lead to setVisible getting called.
        This in an issue because setVisible sets the internalIsInherentlyHidden flag,
        which leads ComponentPeer to think that a window is inherently hidden when it shouldn't be.
        The hack I came up with is to basically set this flag and check it at the end of setVisible
        in order to determine if we're currently inside one of these chain reactions.
        The only other solutions I was able to come up with would have involved rewriting large sections of the windowProc,
        which obviously would not have been good for anyone

        also NSViewComponentPeer has an insideToFrontCall member variable that's being used for some kind of workaround,
        so I'm not the only one doing this :P
    */
    bool insideSetMinimisedCallOrSetVisibleRecursivelyWithoutSettingFlagCall = false;
    bool skipNextCall = false;

    ComponentPeer* peerThatGeneratedFirstHandleBroughtToFrontCall = nullptr;

private:
    //==============================================================================
    virtual void appStyleChanged() {}

    /** Tells the window that text input may be required at the given position.
        This may cause things like a virtual on-screen keyboard to appear, depending
        on the OS.

        This function should not be called directly by Components - use refreshTextInputTarget
        instead.
    */
    virtual void textInputRequired (Point<int>, TextInputTarget&) = 0;

    /** If there's some kind of OS input-method in progress, this should dismiss it.

        Overrides of this function should call closeInputMethodContext().

        This function should not be called directly by Components - use refreshTextInputTarget
        instead.
    */
    virtual void dismissPendingTextInput();

    void globalFocusChanged (Component*) override;
    Component* getTargetForKeyPress();

    void recursivelyRefreshAlwaysOnTopStatus(bool currentPeerIsAlwaysOnTop = false);
    void doSetAlwaysOnTopFalseWorkaround();

   /** Used to support ancestrally always on top peers.
       An ancestrally always on top peer needs to make its native window always on top without changing the value returned by isInherentlyAlwaysOnTop.
       So we need a way to make the native window always on top without setting internalIsInherentlyAlwaysOnTop.

       Important note to any juce people reading this in the future: don't insert an if (alwaysOnTop != currentAlwaysOnTopState) check into any of the platform specific implementations of this function!
       On every platform I've tested on, "redundant" setAlwaysOnTopWithoutSettingFlag calls were necessary in some circumstances in order to get the correct behavior
    */
    virtual bool setAlwaysOnTopWithoutSettingFlag (bool alwaysOnTop) = 0;

    /**
     * Calls setAlwaysOnTopWithoutSettingFlag on this and recursively on all child peers.
    */
    void setAlwaysOnTopRecursivelyWithoutSettingFlag (bool alwaysOnTop);

    void insertIntoFloatingChildPeerList (ComponentPeer* childToBe, int zOrder);

    WeakReference<Component> lastFocusedComponent, dragAndDropTargetComponent;
    Component* lastDragAndDropCompUnderMouse = nullptr;
    TextInputTarget* textInputTarget = nullptr;
    const uint32 uniqueID;
    uint64_t peerFrameNumber = 0;
    bool isWindowMinimised = false;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentPeer)
};

} // namespace juce
