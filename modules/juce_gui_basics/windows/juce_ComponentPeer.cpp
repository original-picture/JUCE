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

#include <stack>


namespace juce
{

static uint32 lastUniquePeerID = 1;


//==============================================================================
ComponentPeer::ComponentPeer (Component& comp, int flags)
    : component (comp),
      styleFlags (flags),
      uniqueID (lastUniquePeerID += 2) // increment by 2 so that this can never hit 0
{
    auto& desktop = Desktop::getInstance();
    desktop.peers.add (this);
    desktop.addFocusChangeListener (this);
}

ComponentPeer::~ComponentPeer()
{
   // if(topLevelParentPeer != nullptr)
   //     topLevelParentPeer->internalRemoveTopLevelChildPeer(this, false);
                                                                  // don't call clearNativeTopLevelParent
                                                                  // (we can't call it here because it's virtual and we're currently in a destructor)
                                                                  // clearNativeTopLevelParent will get called in the destructor of the derived class
    //removeAllTopLevelChildren();

    auto& desktop = Desktop::getInstance();
    desktop.removeFocusChangeListener (this);
    desktop.peers.removeFirstMatchingValue (this);
    desktop.triggerFocusCallback();
}

//==============================================================================
int ComponentPeer::getNumPeers() noexcept
{
    return Desktop::getInstance().peers.size();
}

ComponentPeer* ComponentPeer::getPeer (const int index) noexcept
{
    return Desktop::getInstance().peers [index];
}

ComponentPeer* ComponentPeer::getPeerFor (const Component* const component) noexcept
{
    for (auto* peer : Desktop::getInstance().peers)
        if (&(peer->getComponent()) == component)
            return peer;

    return nullptr;
}

bool ComponentPeer::isValidPeer (const ComponentPeer* const peer) noexcept
{
    return Desktop::getInstance().peers.contains (const_cast<ComponentPeer*> (peer));
}

void ComponentPeer::updateBounds()
{
    setBounds (detail::ScalingHelpers::scaledScreenPosToUnscaled (component, component.getBoundsInParent()), false);
}

bool ComponentPeer::isKioskMode() const
{
    return Desktop::getInstance().getKioskModeComponent() == &component;
}

//==============================================================================
void ComponentPeer::handleMouseEvent (MouseInputSource::InputSourceType type, Point<float> pos, ModifierKeys newMods,
                                      float newPressure, float newOrientation, int64 time, PenDetails pen, int touchIndex)
{
    if (auto* mouse = Desktop::getInstance().mouseSources->getOrCreateMouseInputSource (type, touchIndex))
        MouseInputSource (*mouse).handleEvent (*this, pos, time, newMods, newPressure, newOrientation, pen);
}

void ComponentPeer::handleMouseWheel (MouseInputSource::InputSourceType type, Point<float> pos, int64 time, const MouseWheelDetails& wheel, int touchIndex)
{
    if (auto* mouse = Desktop::getInstance().mouseSources->getOrCreateMouseInputSource (type, touchIndex))
        MouseInputSource (*mouse).handleWheel (*this, pos, time, wheel);
}

void ComponentPeer::handleMagnifyGesture (MouseInputSource::InputSourceType type, Point<float> pos, int64 time, float scaleFactor, int touchIndex)
{
    if (auto* mouse = Desktop::getInstance().mouseSources->getOrCreateMouseInputSource (type, touchIndex))
        MouseInputSource (*mouse).handleMagnifyGesture (*this, pos, time, scaleFactor);
}

//==============================================================================
void ComponentPeer::handlePaint (LowLevelGraphicsContext& contextToPaintTo)
{
    Graphics g (contextToPaintTo);

    if (component.isTransformed())
        g.addTransform (component.getTransform());

    auto peerBounds = getBounds();
    auto componentBounds = component.getLocalBounds();

    if (component.isTransformed())
        componentBounds = componentBounds.transformedBy (component.getTransform());

    if (peerBounds.getWidth() != componentBounds.getWidth() || peerBounds.getHeight() != componentBounds.getHeight())
        // Tweak the scaling so that the component's integer size exactly aligns with the peer's scaled size
        g.addTransform (AffineTransform::scale ((float) peerBounds.getWidth()  / (float) componentBounds.getWidth(),
                                                (float) peerBounds.getHeight() / (float) componentBounds.getHeight()));

  #if JUCE_ENABLE_REPAINT_DEBUGGING
   #ifdef JUCE_IS_REPAINT_DEBUGGING_ACTIVE
    if (JUCE_IS_REPAINT_DEBUGGING_ACTIVE)
   #endif
    {
        g.saveState();
    }
  #endif

    JUCE_TRY
    {
        component.paintEntireComponent (g, true);
    }
    JUCE_CATCH_EXCEPTION

  #if JUCE_ENABLE_REPAINT_DEBUGGING
   #ifdef JUCE_IS_REPAINT_DEBUGGING_ACTIVE
    if (JUCE_IS_REPAINT_DEBUGGING_ACTIVE)
   #endif
    {
        // enabling this code will fill all areas that get repainted with a colour overlay, to show
        // clearly when things are being repainted.
        g.restoreState();

        static Random rng;

        g.fillAll (Colour ((uint8) rng.nextInt (255),
                           (uint8) rng.nextInt (255),
                           (uint8) rng.nextInt (255),
                           (uint8) 0x50));
    }
  #endif

    /** If this fails, it's probably be because your CPU floating-point precision mode has
        been set to low.. This setting is sometimes changed by things like Direct3D, and can
        mess up a lot of the calculations that the library needs to do.
    */
    jassert (roundToInt (10.1f) == 10);

    ++peerFrameNumber;
}

void ComponentPeer::recursivelyRefreshAlwaysOnTopStatus(bool currentNodeIsAlwaysOnTop)
{
    currentNodeIsAlwaysOnTop |= this->isInherentlyAlwaysOnTop(); // since we're traversing the hierarchy from the root, we might as well keep track of the (ancestrally) always on top status ourselves
                                                                 // (as opposed to using isAlwaysOnTop(), which will recursively traverse UP the hierarchy to the root every time
    if (currentNodeIsAlwaysOnTop)               // calling setAlwaysOnTopWithoutSettingFlag(false) is what causes the bug,
        setAlwaysOnTopWithoutSettingFlag(true); // so it's important that we only call setAlwaysOnTopWithoutSettingFlag(true), otherwise we would be undoing our work

    for (auto* peer : topLevelChildPeerList)
    {
        peer->recursivelyRefreshAlwaysOnTopStatus(currentNodeIsAlwaysOnTop);
    }
}

void ComponentPeer::doSetAlwaysOnTopFalseWorkaround()
{
    getHighestLevelAncestor()->recursivelyRefreshAlwaysOnTopStatus();
}

bool ComponentPeer::setAlwaysOnTop (bool alwaysOnTop)
{
    if (setAlwaysOnTopWithoutSettingFlag (alwaysOnTop))
    {
        internalIsInherentlyAlwaysOnTop = alwaysOnTop;

        for (ComponentPeer* child : topLevelChildPeerList)
        {
            child->setAlwaysOnTopRecursivelyWithoutSettingFlag (alwaysOnTop); // I guess technically I should be checking to see if all these recursive calls succeed
        }                                                                     // but the only time setAlwaysOnTopWithoutSettingFlag returns false is on linux,
                                                                              // where it always returns false, so in practice, checking the return value of just one call to setAlwaysOnTopWithoutSettingFlag succeeds is sufficient
                                                                              // though of course all of this could change in the future...
        if(! alwaysOnTop)                                                     // As of 2025/05/21, I've added support for setAlwaysOnTop on linux, so under the current implementation, setAlwaysOnTop actually always returns true...
            doSetAlwaysOnTopFalseWorkaround(); // Windows, linux, and macOS all behave weirdly when calling setAlwaysOnTop (false) on a child window. This workaround makes everything work nicely. See the readme for more details

        return true;
    }
    
    return false;
}

void ComponentPeer::setAlwaysOnTopRecursivelyWithoutSettingFlag (bool alwaysOnTop)
{
    setAlwaysOnTopWithoutSettingFlag (alwaysOnTop);

    if(! this->isInherentlyAlwaysOnTop())
    {
        for(ComponentPeer* peer : topLevelChildPeerList)
        {
            peer->setAlwaysOnTopRecursivelyWithoutSettingFlag (alwaysOnTop);
        }
    }

    // there is no flag member variable for isAncestrallyAlwaysOnTop
    // an always on top ancestor is checked for recursively each time, without any caching
}

bool ComponentPeer::isAlwaysOnTop() const noexcept
{          // short circuit evaluation allows us to avoid calling isAncestrallyAlwaysOnTop() if we don't have to
    return isInherentlyAlwaysOnTop() || isAncestrallyAlwaysOnTop();
                                        // indirect recursion (isAncestrallyAlwaysOnTop() can call isAlwaysOnTop())
}

bool ComponentPeer::isAncestrallyAlwaysOnTop() const noexcept
{

    if(topLevelParentPeer != nullptr)
    {
        return topLevelParentPeer->isAlwaysOnTop(); // indirect recursion (isAlwaysOnTop() can call isAncestrallyAlwaysOnTop())
    }
    else
    {
        return false;
    }
}

bool ComponentPeer::isInherentlyAlwaysOnTop() const noexcept
{
    return internalIsInherentlyAlwaysOnTop;
}

int ComponentPeer::getNumTopLevelChildPeers() const noexcept
{
    return topLevelChildPeerList.size();
}

bool ComponentPeer::addTopLevelChildPeer (ComponentPeer& child, int zOrder)
{
    jassert(! isAttachedToAnotherWindow());       // You tried to add a top level child to this peer when this peer is already attached to another window (using the nativeWindowToAttachTo parameter of Component::addToDesktop)
    jassert(! child.isAttachedToAnotherWindow()); // You tried to add a top level child to this peer when the child-to-be is already attached to another window (using the nativeWindowToAttachTo parameter of Component::addToDesktop)

                                                  // Long story short, nativeWindowToAttachTo and addTopLevelChildPeer map to different systems of the underlying OS-specific APIs
                                                  // For example, nativeWindowToAttachTo creates a win32 *child* window, while addTopLevelChildPeer creates a win32 *owned* window. MacOS and linux have analogous constructs
                                                  // The important thing to know is that these two systems are mutually exclusive. An HWND cannot have a parent AND an owner
                                                  // If you've ended up in a situation where you've attempted to use these two mutually exclusive systems,
                                                  // then you probably want the functionality of one of them, but just don't know which one
                                                  // I would recommend you read up on the underlying OS-specific systems and their behaviors so that you can determine which one is right for your use case

    jassert (this != &child); // adding a peer to itself!?


    if (child.topLevelParentPeer != this) // TODO: add actual cycle detection here?
    {
        if (child.topLevelParentPeer != nullptr)
            child.topLevelParentPeer->removeTopLevelChildPeer (&child);

        if (this->isAlwaysOnTop() && ! child.isAlwaysOnTop())
            child.setAlwaysOnTopRecursivelyWithoutSettingFlag (true); // make child ancestrally always on top

        if (zOrder < 0 || zOrder > topLevelChildPeerList.size())
            zOrder = topLevelChildPeerList.size();

        if (child.isAlwaysOnTop())
        {
            while (zOrder < topLevelChildPeerList.size())
            {
                if (topLevelChildPeerList[zOrder]->isAlwaysOnTop())
                    break;

                ++zOrder;
            }
        }
        else
        {
            while (zOrder > 0)
            {
                if (! topLevelChildPeerList.getUnchecked (zOrder - 1)->isAlwaysOnTop())
                    break;

                --zOrder;
            }
        }

        topLevelChildPeerList.insert (zOrder, &child);

        child.topLevelParentPeer = this;

        child.setNativeTopLevelParent (this);
    }

    return false;
}

void ComponentPeer::removeTopLevelChildPeer (ComponentPeer* childToRemove)
{
    internalRemoveTopLevelChildPeer (childToRemove, true);
}

ComponentPeer* ComponentPeer::removeTopLevelChildPeer (int childIndexToRemove)
{
    return internalRemoveTopLevelChildPeer (childIndexToRemove, true);
}

ComponentPeer* ComponentPeer::internalRemoveTopLevelChildPeer (ComponentPeer* childToRemove, bool shouldCallClearNativeTopLevelParent)
{
    return internalRemoveTopLevelChildPeer(topLevelChildPeerList.indexOf(childToRemove), shouldCallClearNativeTopLevelParent);
}

ComponentPeer* ComponentPeer::internalRemoveTopLevelChildPeer (int childIndexToRemove, bool shouldCallClearNativeTopLevelParent)
{
    if (auto* child = topLevelChildPeerList [childIndexToRemove])
    {
        if (shouldCallClearNativeTopLevelParent)
            child->clearNativeTopLevelParent();

        child->setAlwaysOnTopRecursivelyWithoutSettingFlag(false);
        child->topLevelParentPeer = nullptr;

        topLevelChildPeerList.remove (childIndexToRemove);

        return child;
    }

    return nullptr;
}

void ComponentPeer::doTopLevelChildPeerCleanup()
{
    removeAllTopLevelChildren();

    if (topLevelParentPeer)
        topLevelParentPeer->removeTopLevelChildPeer(this);
}

void ComponentPeer::removeAllTopLevelChildren()
{
    while (! topLevelChildPeerList.isEmpty())
        removeTopLevelChildPeer (topLevelChildPeerList.size() - 1);
}

ComponentPeer* ComponentPeer::getHighestLevelAncestor() noexcept
{
    ComponentPeer* ret = this;
    while (ret->topLevelParentPeer != nullptr)
        ret = ret->topLevelParentPeer;

    return ret;
}

Component* ComponentPeer::getTargetForKeyPress()
{
    auto* c = Component::getCurrentlyFocusedComponent();

    if (c == nullptr)
        c = &component;

    if (c->isCurrentlyBlockedByAnotherModalComponent())
        if (auto* currentModalComp = Component::getCurrentlyModalComponent())
            c = currentModalComp;

    return c;
}

bool ComponentPeer::handleKeyPress (const int keyCode, const juce_wchar textCharacter)
{
    return handleKeyPress (KeyPress (keyCode,
                                     ModifierKeys::currentModifiers.withoutMouseButtons(),
                                     textCharacter));
}

bool ComponentPeer::handleKeyPress (const KeyPress& keyInfo)
{
    bool keyWasUsed = false;

    for (auto* target = getTargetForKeyPress(); target != nullptr; target = target->getParentComponent())
    {
        const WeakReference<Component> deletionChecker (target);

        if (auto* keyListeners = target->keyListeners.get())
        {
            for (int i = keyListeners->size(); --i >= 0;)
            {
                keyWasUsed = keyListeners->getUnchecked (i)->keyPressed (keyInfo, target);

                if (keyWasUsed || deletionChecker == nullptr)
                    return keyWasUsed;

                i = jmin (i, keyListeners->size());
            }
        }

        keyWasUsed = target->keyPressed (keyInfo);

        if (keyWasUsed || deletionChecker == nullptr)
            break;
    }

    if (! keyWasUsed && keyInfo.isKeyCode (KeyPress::tabKey))
    {
        if (auto* currentlyFocused = Component::getCurrentlyFocusedComponent())
        {
            currentlyFocused->moveKeyboardFocusToSibling (! keyInfo.getModifiers().isShiftDown());
            return true;
        }
    }

    return keyWasUsed;
}

bool ComponentPeer::handleKeyUpOrDown (const bool isKeyDown)
{
    bool keyWasUsed = false;

    for (auto* target = getTargetForKeyPress(); target != nullptr; target = target->getParentComponent())
    {
        const WeakReference<Component> deletionChecker (target);

        keyWasUsed = target->keyStateChanged (isKeyDown);

        if (keyWasUsed || deletionChecker == nullptr)
            break;

        if (auto* keyListeners = target->keyListeners.get())
        {
            for (int i = keyListeners->size(); --i >= 0;)
            {
                keyWasUsed = keyListeners->getUnchecked (i)->keyStateChanged (isKeyDown, target);

                if (keyWasUsed || deletionChecker == nullptr)
                    return keyWasUsed;

                i = jmin (i, keyListeners->size());
            }
        }
    }

    return keyWasUsed;
}

void ComponentPeer::handleModifierKeysChange()
{
    auto* target = Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();

    if (target == nullptr)
        target = Component::getCurrentlyFocusedComponent();

    if (target == nullptr)
        target = &component;

    target->internalModifierKeysChanged();
}

void ComponentPeer::refreshTextInputTarget()
{
    const auto* lastTarget = std::exchange (textInputTarget, findCurrentTextInputTarget());

    if (lastTarget == textInputTarget)
        return;

    if (textInputTarget == nullptr)
        dismissPendingTextInput();
    else if (auto* c = Component::getCurrentlyFocusedComponent())
        textInputRequired (globalToLocal (c->getScreenPosition()), *textInputTarget);
}

TextInputTarget* ComponentPeer::findCurrentTextInputTarget()
{
    auto* c = Component::getCurrentlyFocusedComponent();

    if (c == &component || component.isParentOf (c))
        if (auto* ti = dynamic_cast<TextInputTarget*> (c))
            if (ti->isTextInputActive())
                return ti;

    return nullptr;
}

void ComponentPeer::closeInputMethodContext() {}

void ComponentPeer::dismissPendingTextInput()
{
    closeInputMethodContext();
}

//==============================================================================
void ComponentPeer::handleBroughtToFront()
{
    component.internalBroughtToFront();
}

void ComponentPeer::setConstrainer (ComponentBoundsConstrainer* const newConstrainer) noexcept
{
    constrainer = newConstrainer;
}

void ComponentPeer::handleMovedOrResized()
{
    const bool nowMinimised = isMinimised();

    if (component.flags.hasHeavyweightPeerFlag && ! nowMinimised)
    {
        const WeakReference<Component> deletionChecker (&component);

        auto newBounds = detail::ComponentHelpers::rawPeerPositionToLocal (component, getBounds());
        auto oldBounds = component.getBounds();

        const bool wasMoved   = (oldBounds.getPosition() != newBounds.getPosition());
        const bool wasResized = (oldBounds.getWidth() != newBounds.getWidth() || oldBounds.getHeight() != newBounds.getHeight());

        if (wasMoved || wasResized)
        {
            component.boundsRelativeToParent = newBounds;

            if (wasResized)
                component.repaint();

            component.sendMovedResizedMessages (wasMoved, wasResized);

            if (deletionChecker == nullptr)
                return;
        }
    }

    if (isWindowMinimised != nowMinimised)
    {
        isWindowMinimised = nowMinimised;
        component.minimisationStateChanged (nowMinimised);
        component.sendVisibilityChangeMessage();
    }

    const auto windowInSpecialState = isFullScreen() || isKioskMode() || nowMinimised;

    if (! windowInSpecialState)
        lastNonFullscreenBounds = component.getBounds();
}

void ComponentPeer::handleFocusGain()
{
    if (component.isParentOf (lastFocusedComponent)
          && lastFocusedComponent->isShowing()
          && lastFocusedComponent->getWantsKeyboardFocus())
    {
        Component::currentlyFocusedComponent = lastFocusedComponent;
        Desktop::getInstance().triggerFocusCallback();
        lastFocusedComponent->internalKeyboardFocusGain (Component::focusChangedDirectly);
    }
    else
    {
        if (! component.isCurrentlyBlockedByAnotherModalComponent())
            component.grabKeyboardFocus();
        else
            ModalComponentManager::getInstance()->bringModalComponentsToFront();
    }
}

void ComponentPeer::handleFocusLoss()
{
    if (component.hasKeyboardFocus (true))
    {
        lastFocusedComponent = Component::currentlyFocusedComponent;

        if (lastFocusedComponent != nullptr)
        {
            Component::currentlyFocusedComponent = nullptr;
            Desktop::getInstance().triggerFocusCallback();
            lastFocusedComponent->internalKeyboardFocusLoss (Component::focusChangedByMouseClick);
        }
    }
}

Component* ComponentPeer::getLastFocusedSubcomponent() const noexcept
{
    return (component.isParentOf (lastFocusedComponent) && lastFocusedComponent->isShowing())
                ? static_cast<Component*> (lastFocusedComponent)
                : &component;
}

void ComponentPeer::handleScreenSizeChange()
{
    component.parentSizeChanged();
    handleMovedOrResized();
}

void ComponentPeer::setNonFullScreenBounds (const Rectangle<int>& newBounds) noexcept
{
    lastNonFullscreenBounds = newBounds;
}

const Rectangle<int>& ComponentPeer::getNonFullScreenBounds() const noexcept
{
    return lastNonFullscreenBounds;
}

Point<int> ComponentPeer::localToGlobal (Point<int> p)   { return localToGlobal (p.toFloat()).roundToInt(); }
Point<int> ComponentPeer::globalToLocal (Point<int> p)   { return globalToLocal (p.toFloat()).roundToInt(); }

Rectangle<int> ComponentPeer::localToGlobal (const Rectangle<int>& relativePosition)
{
    return relativePosition.withPosition (localToGlobal (relativePosition.getPosition()));
}

Rectangle<int> ComponentPeer::globalToLocal (const Rectangle<int>& screenPosition)
{
    return screenPosition.withPosition (globalToLocal (screenPosition.getPosition()));
}

Rectangle<float> ComponentPeer::localToGlobal (const Rectangle<float>& relativePosition)
{
    return relativePosition.withPosition (localToGlobal (relativePosition.getPosition()));
}

Rectangle<float> ComponentPeer::globalToLocal (const Rectangle<float>& screenPosition)
{
    return screenPosition.withPosition (globalToLocal (screenPosition.getPosition()));
}

Rectangle<int> ComponentPeer::getAreaCoveredBy (const Component& subComponent) const
{
    return detail::ScalingHelpers::scaledScreenPosToUnscaled
            (component, component.getLocalArea (&subComponent, subComponent.getLocalBounds()));
}

//=============================================================================
void ComponentPeer::setMinimised (bool shouldBeMinimised)
{
    if (! shouldBeMinimised && topLevelParentPeer != nullptr && topLevelParentPeer->isMinimised()) // this code makes sure that a peer's parents are deminimised before it itself gets deminimised
    {                                                                                              // basically, if you deminimise a window that has a minimised parent, you have to walk up the window hierarchy until you find either a window that isn't minimised or you reach the root of the hierarchy,
        std::stack<ComponentPeer*> peersToProcess;                                                 // pushing peers onto a stack as you go.

        { // limit the scope of peer
            ComponentPeer* peer = this;
            while (((peer = peer->topLevelParentPeer) != nullptr) && peer->isMinimised()) // Note that "this" does NOT get pushed to the stack. this peer gets processed separately below
                peersToProcess.push (peer);
        }

        while (! peersToProcess.empty()) // then you pop each one off the stack and deminimise it.
        {
            auto* peer = peersToProcess.top();
            peersToProcess.pop();

            peer->setMinimised (false);
        }
    }

    #ifdef __APPLE__
        setMinimisedWithoutSettingFlag (shouldBeMinimised); // miniaturisation on macOS works differently from minimisation on windows and most linux desktop environments
                                                            // miniaturised windows are visible as individual icons on the dock, so recursively calling setMinimised (which does the right thing on windows and linux)
                                                            // would spit every window in the hierarchy onto the users dock. This is not desirable, so we avoid the recursive setMinimised calls on macOS
    #else
        // setMinimisedRecursivelyWithoutSettingFlag (shouldBeMinimised);
        setMinimisedWithoutSettingFlag(shouldBeMinimised);
        for (ComponentPeer* peer : topLevelChildPeerList)
        {

        }
    #endif

    internalIsInherentlyMinimised = shouldBeMinimised;
}

void ComponentPeer::setMinimisedRecursivelyWithoutSettingFlag (bool shouldBeMinimised)
{
    if (! shouldBeMinimised)                                // This if statement and the if statement at the end of the function make the traversal preorder if we're restoring the window (shouldBeMinimised is false),
        setMinimisedWithoutSettingFlag (shouldBeMinimised); // and postorder if we're minimising it.

    for (auto* peer : topLevelChildPeerList)
    {                                                                      // don't accidentally deminimise an inherently minimised window
        peer->setMinimisedRecursivelyWithoutSettingFlag (shouldBeMinimised || peer->isInherentlyMinimised());
    }

    if (shouldBeMinimised)
        setMinimisedWithoutSettingFlag (shouldBeMinimised);
}

bool ComponentPeer::isMinimised() const noexcept
{          // short circuit evaluation allows us to avoid calling isAncestrallyMinimised() if we don't have to
    return isInherentlyMinimised() || isAncestrallyMinimised();
                                      // indirect recursion (isAncestrallyMinimised() can call isMinimised())
}

bool ComponentPeer::isAncestrallyMinimised() const noexcept
{

    if(topLevelParentPeer != nullptr)
    {
        return topLevelParentPeer->isMinimised(); // indirect recursion (isMinimised() can call isAncestrallyMinimised())
    }
    else
    {
        return false;
    }
}

bool ComponentPeer::isInherentlyMinimised() const noexcept
{
    return internalIsInherentlyMinimised;
}

//==============================================================================
namespace DragHelpers
{
    static bool isFileDrag (const ComponentPeer::DragInfo& info)
    {
        return ! info.files.isEmpty();
    }

    static bool isSuitableTarget (const ComponentPeer::DragInfo& info, Component* target)
    {
        return isFileDrag (info) ? dynamic_cast<FileDragAndDropTarget*> (target) != nullptr
                                 : dynamic_cast<TextDragAndDropTarget*> (target) != nullptr;
    }

    static bool isInterested (const ComponentPeer::DragInfo& info, Component* target)
    {
        return isFileDrag (info) ? dynamic_cast<FileDragAndDropTarget*> (target)->isInterestedInFileDrag (info.files)
                                 : dynamic_cast<TextDragAndDropTarget*> (target)->isInterestedInTextDrag (info.text);
    }

    static Component* findDragAndDropTarget (Component* c, const ComponentPeer::DragInfo& info, Component* lastOne)
    {
        for (; c != nullptr; c = c->getParentComponent())
            if (isSuitableTarget (info, c) && (c == lastOne || isInterested (info, c)))
                return c;

        return nullptr;
    }
}

bool ComponentPeer::handleDragMove (const ComponentPeer::DragInfo& info)
{
    auto* compUnderMouse = component.getComponentAt (info.position);
    auto* lastTarget = dragAndDropTargetComponent.get();
    Component* newTarget = nullptr;

    if (compUnderMouse != lastDragAndDropCompUnderMouse)
    {
        lastDragAndDropCompUnderMouse = compUnderMouse;
        newTarget = DragHelpers::findDragAndDropTarget (compUnderMouse, info, lastTarget);

        if (newTarget != lastTarget)
        {
            if (lastTarget != nullptr)
            {
                if (DragHelpers::isFileDrag (info))
                    dynamic_cast<FileDragAndDropTarget*> (lastTarget)->fileDragExit (info.files);
                else
                    dynamic_cast<TextDragAndDropTarget*> (lastTarget)->textDragExit (info.text);
            }

            dragAndDropTargetComponent = nullptr;

            if (DragHelpers::isSuitableTarget (info, newTarget))
            {
                dragAndDropTargetComponent = newTarget;
                auto pos = newTarget->getLocalPoint (&component, info.position);

                if (DragHelpers::isFileDrag (info))
                    dynamic_cast<FileDragAndDropTarget*> (newTarget)->fileDragEnter (info.files, pos.x, pos.y);
                else
                    dynamic_cast<TextDragAndDropTarget*> (newTarget)->textDragEnter (info.text, pos.x, pos.y);
            }
        }
    }
    else
    {
        newTarget = lastTarget;
    }

    if (! DragHelpers::isSuitableTarget (info, newTarget))
        return false;

    auto pos = newTarget->getLocalPoint (&component, info.position);

    if (DragHelpers::isFileDrag (info))
        dynamic_cast<FileDragAndDropTarget*> (newTarget)->fileDragMove (info.files, pos.x, pos.y);
    else
        dynamic_cast<TextDragAndDropTarget*> (newTarget)->textDragMove (info.text, pos.x, pos.y);

    return true;
}

bool ComponentPeer::handleDragExit (const ComponentPeer::DragInfo& info)
{
    DragInfo info2 (info);
    info2.position.setXY (-1, -1);
    const bool used = handleDragMove (info2);

    jassert (dragAndDropTargetComponent == nullptr);
    lastDragAndDropCompUnderMouse = nullptr;
    return used;
}

bool ComponentPeer::handleDragDrop (const ComponentPeer::DragInfo& info)
{
    handleDragMove (info);

    if (WeakReference<Component> targetComp = dragAndDropTargetComponent)
    {
        dragAndDropTargetComponent = nullptr;
        lastDragAndDropCompUnderMouse = nullptr;

        if (DragHelpers::isSuitableTarget (info, targetComp))
        {
            if (targetComp->isCurrentlyBlockedByAnotherModalComponent())
            {
                targetComp->internalModalInputAttempt();

                if (targetComp->isCurrentlyBlockedByAnotherModalComponent())
                    return true;
            }

            ComponentPeer::DragInfo infoCopy (info);
            infoCopy.position = targetComp->getLocalPoint (&component, info.position);

            // We'll use an async message to deliver the drop, because if the target decides
            // to run a modal loop, it can gum-up the operating system..
            MessageManager::callAsync ([=]
            {
                if (auto* c = targetComp.get())
                {
                    if (DragHelpers::isFileDrag (info))
                        dynamic_cast<FileDragAndDropTarget*> (c)->filesDropped (infoCopy.files, infoCopy.position.x, infoCopy.position.y);
                    else
                        dynamic_cast<TextDragAndDropTarget*> (c)->textDropped (infoCopy.text, infoCopy.position.x, infoCopy.position.y);
                }
            });

            return true;
        }
    }

    return false;
}

//==============================================================================
void ComponentPeer::handleUserClosingWindow()
{
    component.userTriedToCloseWindow();
}

bool ComponentPeer::setDocumentEditedStatus (bool)
{
    return false;
}

void ComponentPeer::setRepresentedFile (const File&)
{
}

//==============================================================================
int ComponentPeer::getCurrentRenderingEngine() const                             { return 0; }
void ComponentPeer::setCurrentRenderingEngine ([[maybe_unused]] int index)       { jassert (index == 0); }

//==============================================================================
std::function<ModifierKeys()> ComponentPeer::getNativeRealtimeModifiers = nullptr;

ModifierKeys ComponentPeer::getCurrentModifiersRealtime() noexcept
{
    if (getNativeRealtimeModifiers != nullptr)
        return getNativeRealtimeModifiers();

    return ModifierKeys::currentModifiers;
}

//==============================================================================
void ComponentPeer::forceDisplayUpdate()
{
    Desktop::getInstance().displays->refresh();
}

void ComponentPeer::callVBlankListeners (double timestampSec)
{
    vBlankListeners.call ([timestampSec] (auto& l) { l.onVBlank (timestampSec); });
}

void ComponentPeer::globalFocusChanged ([[maybe_unused]] Component* comp)
{
    refreshTextInputTarget();
}

} // namespace juce
