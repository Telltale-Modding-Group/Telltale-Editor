#pragma once

#include <Core/Math.hpp>
#include <Core/Symbol.hpp>

#include <memory>

class PlaybackController;

/**
 * Runtime renderable text from module text / text2.
 */
class RenderText
{
public:

    // Getters
    bool GetWorldSpaceZ() const;
    float GetWidth() const;
    VerticalAlignmentType GetVerticalAlignment() const;
    HorizontalAlignmentType GetHorizontalAlignment() const;
    float GetAlphaScale() const;
    const String& GetText() const;
    float GetSkew() const;
    float GetShadowHeight() const;
    const Colour& GetShadowColour() const;
    const Vector3& GetNonPropScale() const;
    float GetScale() const;
    const Vector2& GetRefScreenSize() const;
    float GetPlaybackSpeed() const;
    const Vector3& GetOffset() const;
    float GetPercentToDisplay() const;
    float GetMinWidth() const;
    float GetMinHeight() const;
    I32 GetMaxLinesToDisplay() const;
    float GetLeading() const;
    float GetKerning() const;
    const Symbol& GetFontFile() const;
    float GetExtrudeX() const;
    float GetExtrudeY() const;
    const String& GetDialogNodeName() const;
    const Symbol& GetDialogFile() const;
    const String& GetDialogTextResource() const;
    const Symbol& GetDialogResFile() const;
    I32 GetCurrentDisplayPage() const;
    const Colour& GetTextColour() const;
    const Colour& GetTextBackgroundColor() const;
    float GetTextBackgroundAlphaScale() const;

    // Setters
    void SetWorldSpaceZ(bool value);
    void SetWidth(float value);
    void SetVerticalAlignment(Enum<VerticalAlignmentType> value);
    void SetHorizontalAlignment(Enum<HorizontalAlignmentType> value);
    void SetAlphaScale(float value);
    void SetText(String value);
    void SetSkew(float value);
    void SetShadowHeight(float value);
    void SetShadowColour(Colour value);
    void SetNonPropScale(Vector3 value);
    void SetScale(float value);
    void SetRefScreenSize( Vector2 value);
    void SetPlaybackSpeed(float value);
    void SetOffset(Vector3 value);
    void SetPercentToDisplay(float value);
    void SetMinWidth(float value);
    void SetMinHeight(float value);
    void SetMaxLinesToDisplay(I32 value);
    void SetLeading(float value);
    void SetKerning(float value);
    void SetFontFile(Symbol value);
    void SetExtrudeX(float value);
    void SetExtrudeY(float value);
    void SetDialogNodeName(String value);
    void SetDialogFile(Symbol value);
    void SetDialogTextResource(String value);
    void SetDialogResFile(Symbol value);
    void SetCurrentDisplayPage(I32 value);
    void SetTextColour(Colour value);
    void SetTextBackgroundColor(Colour value);
    void SetTextBackgroundAlphaScale(float value);

private:

    bool _WorldSpaceZ = false;
    bool _BackgroundEnabled = true; // not in newer games, but old games switch this off as they use it, so keep it true
    float _Width = 0.0f;
    VerticalAlignmentType _VAlignType;
    HorizontalAlignmentType _HAlignType;
    float _AlphaScale = 1.0f;
    String _Text; // actual text
    float _Skew = 0.0f;
    float _ShadowHeight = 0.0f;
    Colour _ShadowColour = Colour::Black;
    Vector3 _NonPropScale = Vector3::Identity;
    float _Scale = 1.0f;
    Vector2 _RefScreenSize = Vector2(800.0f, 450.0f);
    float _PlaybackSpeed = 0.f;
    Vector3 _Offset;
    float _PercentToDisplay = 1.0f;
    float _MinWidth = 0.0f;
    float _MinHeight = 0.0f;
    I32 _MaxLinesToDisplay = 2; // max 2 lines default
    float _Leading = 1.0f;
    float _Kerning = 1.0f;
    Symbol _FontFile;
    float _ExtrudeX = 20.0f;
    float _ExtrudeY = 20.0f;
    String _DialogNodeName;
    Symbol _DialogFile; // Dlg class
    String _DialogTextResource;
    Symbol _DialogResFile; // older DialogResource class
    I32 _CurrentDisplayPage = 0;
    Colour _TextColour = Colour::White;
    Colour _TextBackgroundColor = Colour::DarkRed;
    float _TextBackgroundAlphaScale = 1.0f;

};
