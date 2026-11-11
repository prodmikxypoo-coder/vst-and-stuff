#include "PluginEditor.h"

namespace
{
    juce::Colour mixC (juce::uint32 a, juce::uint32 b, float t)
    {
        return juce::Colour (a).interpolatedWith (juce::Colour (b), juce::jlimit (0.0f, 1.0f, t));
    }

    float smoothStep (float x, float a, float b)
    {
        const float k = juce::jlimit (0.0f, 1.0f, (x - a) / (b - a));
        return k * k * (3.0f - 2.0f * k);
    }

    // k < 1 darkens, k > 1 lightens toward white
    juce::Colour tint (juce::Colour c, float k)
    {
        if (k <= 1.0f)
            return juce::Colour::fromFloatRGBA (c.getFloatRed() * k, c.getFloatGreen() * k,
                                                c.getFloatBlue() * k, c.getFloatAlpha());
        return c.interpolatedWith (juce::Colours::white, juce::jlimit (0.0f, 1.0f, k - 1.0f))
                .withAlpha (c.getFloatAlpha());
    }

    // Lit sphere: highlight upper-left, dark rim
    void fillSphere (juce::Graphics& g, float x, float y, float rx, float ry,
                     juce::Colour base, float hiK, float dkK)
    {
        const float m = juce::jmax (rx, ry);
        const float hx = x - 0.35f * rx;
        const float hy = y - 0.4f * ry;
        juce::ColourGradient gr (tint (base, hiK), hx, hy, tint (base, dkK), hx + m * 1.5f, hy, true);
        gr.addColour (0.4, base);
        g.setGradientFill (gr);
        g.fillEllipse (x - rx, y - ry, rx * 2.0f, ry * 2.0f);
    }

    void softShadow (juce::Graphics& g, const juce::Path& p, juce::Colour c, int radius, int ox, int oy)
    {
        juce::DropShadow ds (c, radius, { ox, oy });
        ds.drawForPath (g, p);
    }

    juce::Path ellipsePath (float x, float y, float rx, float ry)
    {
        juce::Path p;
        p.addEllipse (x - rx, y - ry, rx * 2.0f, ry * 2.0f);
        return p;
    }

    void faceTri (juce::Graphics& g, float x1, float y1, float x2, float y2, float x3, float y3,
                  juce::Colour c1, float gx1, float gy1, juce::Colour c2, float gx2, float gy2)
    {
        juce::Path p;
        p.addTriangle (x1, y1, x2, y2, x3, y3);
        g.setGradientFill (juce::ColourGradient (c1, gx1, gy1, c2, gx2, gy2, false));
        g.fillPath (p);
    }

    // Custom 3D-looking rotary knob
    class PandaKnobLook : public juce::LookAndFeel_V4
    {
    public:
        float v = 0.0f;

        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                               float startA, float endA, juce::Slider&) override
        {
            const float cx = (float) x + (float) width * 0.5f;
            const float cy = (float) y + (float) height * 0.5f;
            const float ang = startA + pos * (endA - startA);

            juce::Path track;
            track.addCentredArc (cx, cy, 66.0f, 66.0f, 0.0f, startA, endA, true);
            g.setColour (juce::Colour (0xff050507));
            g.strokePath (track, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            if (pos > 0.001f)
            {
                juce::Path act;
                act.addCentredArc (cx, cy, 66.0f, 66.0f, 0.0f, startA, ang, true);
                juce::DropShadow glow (mixC (0xffff7a1a, 0xffff2010, v).withAlpha (0.8f), 12, { 0, 0 });
                glow.drawForPath (g, act);
                g.setColour (mixC (0xffff7a1a, 0xffd01818, v));
                g.strokePath (act, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
            }

            const auto kb = mixC (0xff6a6a76, 0xff7a3a3a, v);
            softShadow (g, ellipsePath (cx, cy, 52.0f, 52.0f), juce::Colours::black.withAlpha (0.7f), 12, 0, 7);
            fillSphere (g, cx, cy, 52.0f, 52.0f, kb, 1.7f, 0.22f);
            g.setColour (juce::Colour (0xffd2d2e1).withAlpha (0.4f));
            g.drawEllipse (cx - 51.0f, cy - 51.0f, 102.0f, 102.0f, 1.5f);

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            for (int i = 0; i < 36; ++i)
            {
                const float a = (float) i / 36.0f * juce::MathConstants<float>::twoPi;
                g.drawLine (cx + std::cos (a) * 46.0f, cy + std::sin (a) * 46.0f,
                            cx + std::cos (a) * 51.0f, cy + std::sin (a) * 51.0f, 1.5f);
            }

            fillSphere (g, cx, cy, 32.0f, 32.0f, mixC (0xff4a4a54, 0xff4a1818, v), 1.6f, 0.3f);

            const float sx = std::sin (ang), sy = -std::cos (ang);
            juce::Path ind;
            ind.startNewSubPath (cx + sx * 14.0f, cy + sy * 14.0f);
            ind.lineTo (cx + sx * 44.0f, cy + sy * 44.0f);
            juce::DropShadow ig (juce::Colour (0xffff3020).withAlpha (0.5f + 0.4f * v), 8, { 0, 0 });
            ig.drawForPath (g, ind);
            g.setColour (juce::Colours::white);
            g.strokePath (ind, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }
    };
}

PandaAudioProcessorEditor::PandaAudioProcessorEditor (PandaAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), rageAttach (p.apvts, "rage", rageSlider)
{
    look = std::make_unique<PandaKnobLook>();
    rageSlider.setLookAndFeel (look.get());
    rageSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    rageSlider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f, true);
    rageSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (rageSlider);

    setSize (480, 600);
    startTimerHz (30);
}

PandaAudioProcessorEditor::~PandaAudioProcessorEditor()
{
    stopTimer();
    rageSlider.setLookAndFeel (nullptr);
}

void PandaAudioProcessorEditor::resized()
{
    rageSlider.setBounds (getWidth() / 2 - 75, getHeight() - 190, 150, 150);
}

void PandaAudioProcessorEditor::timerCallback()
{
    const float target = (float) rageSlider.getValue();
    vis += (target - vis) * 0.2f;
    static_cast<PandaKnobLook*> (look.get())->v = vis;
    ++tick;
    repaint();
}

void PandaAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    const float v = vis;
    const float t = (float) tick * 0.1f;
    const float pi = juce::MathConstants<float>::pi;

    const float kBlood = smoothStep (v, 0.4f, 1.0f);
    const float kArmor = smoothStep (v, 0.45f, 0.9f);
    const float kCrown = smoothStep (v, 0.25f, 0.8f);
    const float kBeard = smoothStep (v, 0.4f, 0.85f);
    const float kTears = smoothStep (v, 0.45f, 1.0f);

    const juce::Colour ink (0xff17171a);

    // ---- Background spotlight ----
    {
        juce::ColourGradient bg (mixC (0xffe8fbff, 0xff3a0a12, v), w * 0.5f, h * 0.35f,
                                 mixC (0xff8fd3ee, 0xff040002, v), w * 0.5f + 420.0f, h * 0.35f, true);
        g.setGradientFill (bg);
        g.fillRect (getLocalBounds());
    }

    // Bamboo (cylinder-shaded)
    for (int i = 0; i < 6; ++i)
    {
        const float x = 20.0f + (float) i * 88.0f;
        const auto b = mixC (0xff5ccb5f, 0xff1c0810, v);
        juce::ColourGradient bg (tint (b, 0.55f), x, 0.0f, tint (b, 0.5f), x + 14.0f, 0.0f, false);
        bg.addColour (0.35, tint (b, 1.35f));
        g.setGradientFill (bg);
        g.fillRect (x, 0.0f, 14.0f, h);
        g.setColour (tint (mixC (0xff3ea845, 0xff0e0408, v), 0.8f));
        for (float y = 60.0f + (float) (i % 3) * 30.0f; y < h; y += 110.0f)
            g.fillRect (x - 2.0f, y, 18.0f, 5.0f);
    }

    // Confetti
    if (v < 0.97f)
    {
        juce::Random cr (1234);
        for (int i = 0; i < 40; ++i)
        {
            const float x = cr.nextFloat() * w;
            const float y = cr.nextFloat() * h * 0.8f + std::sin (t + (float) i) * 6.0f;
            const float s = 5.0f + cr.nextFloat() * 8.0f;
            const auto c = juce::Colour::fromHSV (cr.nextFloat(), 0.7f, 0.95f, (1.0f - v) * 0.85f);
            fillSphere (g, x, y, s * 0.5f, s * 0.5f, c, 1.6f, 0.6f);
        }
    }

    // Grunge ink-wash blobs
    {
        juce::Random br (55);
        for (int i = 0; i < 8; ++i)
        {
            const float x = br.nextFloat() * w;
            const float y = h * 0.45f + br.nextFloat() * h * 0.55f;
            const float rx = 40.0f + br.nextFloat() * 80.0f;
            const float ry = 20.0f + br.nextFloat() * 40.0f;
            g.setColour (juce::Colour (0xffb9b2a2).withAlpha (0.10f * v));
            g.fillEllipse (x - rx, y - ry, rx * 2.0f, ry * 2.0f);
        }
    }

    // Blood wash + glossy drips
    if (kBlood > 0.01f)
    {
        juce::Path wash;
        wash.startNewSubPath (0.0f, 0.0f);
        wash.lineTo (w, 0.0f);
        for (float x = w; x >= 0.0f; x -= 20.0f)
            wash.lineTo (x, 70.0f * kBlood + std::sin (x * 0.05f) * 14.0f * kBlood);
        wash.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7a1010).withAlpha (0.95f * kBlood), 0.0f, 0.0f,
                                                 juce::Colour (0xff2a0404).withAlpha (0.95f * kBlood), 0.0f, 90.0f, false));
        g.fillPath (wash);

        juce::Random dr (77);
        for (int i = 0; i < 10; ++i)
        {
            const float x = 20.0f + dr.nextFloat() * (w - 40.0f);
            const float len = 60.0f + dr.nextFloat() * 240.0f;
            const float dw = 6.0f + dr.nextFloat() * 8.0f;
            const float L = len * kBlood + std::sin (t * 0.6f + (float) i) * 3.0f * kBlood;

            juce::ColourGradient dg (juce::Colour (0xff2a0404), x - dw * 0.5f, 0.0f,
                                     juce::Colour (0xff3a0606), x + dw * 0.5f, 0.0f, false);
            dg.addColour (0.35, juce::Colour (0xff9a2222));
            g.setGradientFill (dg);
            g.fillRect (x - dw * 0.5f, 0.0f, dw, L);
            g.fillEllipse (x - dw * 0.7f, L - dw * 0.3f, dw * 1.4f, dw * 1.9f);
            g.setColour (juce::Colour (0xffffc8c8).withAlpha (0.6f));
            g.fillEllipse (x - dw * 0.32f, L + dw * 0.0f, dw * 0.24f, dw * 0.4f);
        }
    }

    // ---- Panda geometry ----
    float cx = w * 0.5f;
    float cy = h * 0.38f + std::sin (t) * 4.0f * (1.0f - v);
    const float R = 105.0f * (1.0f + 0.3f * v);

    if (v > 0.5f)   // rage shake
    {
        cx += (rng.nextFloat() - 0.5f) * v * 6.0f;
        cy += (rng.nextFloat() - 0.5f) * v * 6.0f;
    }

    // ---- Soft blood-red back glow ----
    if (v > 0.02f)
    {
        juce::ColourGradient aura (juce::Colour (0xffa00a0a).withAlpha (0.45f * v), cx, cy - R * 0.2f,
                                   juce::Colour (0xffa00a0a).withAlpha (0.0f), cx + R * 2.4f, cy - R * 0.2f, true);
        aura.addColour (0.3, juce::Colour (0xffa00a0a).withAlpha (0.35f * v));
        g.setGradientFill (aura);
        g.fillEllipse (cx - R * 2.4f, cy - R * 0.2f - R * 2.4f, R * 4.8f, R * 4.8f);
    }

    // ---- Contact shadow under the head ----
    {
        g.saveState();
        g.addTransform (juce::AffineTransform::scale (1.0f, 0.35f, cx, cy + R * 0.95f));
        juce::ColourGradient cs (juce::Colours::black.withAlpha (0.5f * (0.3f + 0.7f * kArmor)), cx, cy + R * 0.95f,
                                 juce::Colours::black.withAlpha (0.0f), cx + R, cy + R * 0.95f, true);
        g.setGradientFill (cs);
        g.fillEllipse (cx - R, cy + R * 0.95f - R, R * 2.0f, R * 2.0f);
        g.restoreState();
    }

    // ---- Spiked armor: pauldrons + chest plate ----
    if (kArmor > 0.01f)
    {
        g.setOpacity (1.0f);
        for (int side = -1; side <= 1; side += 2)
        {
            const float s = (float) side;
            auto P = [&] (float fx, float fy) { return juce::Point<float> (cx + s * R * fx, cy + R * fy); };
            const auto a = P (0.5f, 0.6f);
            const auto b = P (1.75f, 1.5f);

            juce::Path sh;
            sh.startNewSubPath (P (0.5f, 0.8f));
            sh.lineTo (P (1.1f, 0.55f));
            sh.lineTo (P (1.65f, 0.78f));
            sh.lineTo (P (1.75f, 1.5f));
            sh.lineTo (P (0.4f, 1.4f));
            sh.closeSubPath();
            juce::ColourGradient og (juce::Colour (0xff5a5a66).withAlpha (kArmor), a.x, a.y,
                                     juce::Colour (0xff060608).withAlpha (kArmor), b.x, b.y, false);
            og.addColour (0.35, juce::Colour (0xff1d1d24).withAlpha (kArmor));
            g.setGradientFill (og);
            g.fillPath (sh);
            g.setColour (juce::Colour (0xff8a8a98).withAlpha (kArmor));
            g.strokePath (sh, juce::PathStrokeType (2.0f));

            juce::Path inner;
            inner.startNewSubPath (P (0.62f, 0.9f));
            inner.lineTo (P (1.1f, 0.69f));
            inner.lineTo (P (1.5f, 0.88f));
            inner.lineTo (P (1.58f, 1.32f));
            inner.lineTo (P (0.55f, 1.26f));
            inner.closeSubPath();
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c34).withAlpha (kArmor), a.x, a.y,
                                                     juce::Colour (0xff0a0a0e).withAlpha (kArmor), b.x, b.y, false));
            g.fillPath (inner);
            g.setColour (juce::Colour (0xff5a5a66).withAlpha (kArmor));
            g.strokePath (inner, juce::PathStrokeType (1.5f));

            const float rv[3][2] = { { 0.85f, 1.0f }, { 1.15f, 0.86f }, { 1.42f, 1.06f } };
            for (auto& r : rv)
            {
                const auto p = P (r[0], r[1]);
                fillSphere (g, p.x, p.y, 4.0f, 4.0f, juce::Colour (0xff6a6a76).withAlpha (kArmor), 1.8f, 0.3f);
            }

            for (int k = 0; k < 4; ++k)
            {
                const float f = (float) k / 3.0f;
                const auto bb = P (1.1f + 0.55f * f, 0.55f + 0.23f * f);
                const float tx = bb.x + s * 10.0f;
                const float ty = bb.y - 34.0f * kArmor;
                faceTri (g, bb.x - 8.0f, bb.y + 2.0f, tx, ty, bb.x, bb.y + 2.0f,
                         juce::Colour (0xff5a5a66).withAlpha (kArmor), bb.x - 8.0f, bb.y,
                         juce::Colour (0xffd0d0dc).withAlpha (kArmor), tx, ty);
                faceTri (g, bb.x, bb.y + 2.0f, tx, ty, bb.x + 8.0f, bb.y + 2.0f,
                         juce::Colour (0xff101016).withAlpha (kArmor), bb.x, bb.y,
                         juce::Colour (0xff101016).withAlpha (kArmor), tx, ty);
            }
        }

        juce::Path chest;
        chest.startNewSubPath (cx - R * 0.5f, cy + R * 0.85f);
        chest.lineTo (cx + R * 0.5f, cy + R * 0.85f);
        chest.lineTo (cx + R * 0.35f, cy + R * 1.5f);
        chest.lineTo (cx - R * 0.35f, cy + R * 1.5f);
        chest.closeSubPath();
        juce::ColourGradient cg (juce::Colour (0xff08080b).withAlpha (kArmor), cx - R * 0.5f, 0.0f,
                                 juce::Colour (0xff040406).withAlpha (kArmor), cx + R * 0.5f, 0.0f, false);
        cg.addColour (0.38, juce::Colour (0xff3e3e48).withAlpha (kArmor));
        cg.addColour (0.55, juce::Colour (0xff18181e).withAlpha (kArmor));
        g.setGradientFill (cg);
        g.fillPath (chest);
        g.setColour (juce::Colour (0xff6a6a76).withAlpha (kArmor));
        g.strokePath (chest, juce::PathStrokeType (2.0f));
        g.setColour (juce::Colour (0xff8a8a98).withAlpha (kArmor));
        g.drawLine (cx, cy + R * 0.9f, cx, cy + R * 1.5f, 1.5f);
    }

    // ---- Ears (lit spheres) ----
    for (int side = -1; side <= 1; side += 2)
    {
        const float er = R * 0.32f;
        const float ex = cx + (float) side * R * 0.72f;
        const float ey = cy - R * 0.72f;
        softShadow (g, ellipsePath (ex, ey, er, er), juce::Colours::black.withAlpha (0.5f), 14, 0, 6);
        fillSphere (g, ex, ey, er, er, juce::Colour (0xff262626), 1.7f, 0.3f);
        g.setColour (juce::Colour (0xff967887).withAlpha (0.35f));
        g.fillEllipse (ex - (float) side * R * 0.02f - er * 0.55f, ey + R * 0.02f - er * 0.55f, er * 1.1f, er * 1.1f);
    }

    // ---- Head: lit sphere with drop shadow, rim light and fur ----
    {
        const auto hb = mixC (0xffffffff, 0xff9d9da3, v);
        const float hd = 0.82f - 0.37f * v;
        const auto headPath = ellipsePath (cx, cy, R, R * 0.92f);

        softShadow (g, headPath, juce::Colours::black.withAlpha (0.55f), 28, 0, 14);

        juce::ColourGradient hg (tint (hb, 1.3f), cx - 0.35f * R, cy - 0.45f * R * 0.92f,
                                 tint (hb, hd), cx - 0.35f * R + R * 1.9f, cy - 0.45f * R * 0.92f, true);
        hg.addColour (0.4, hb);
        g.setGradientFill (hg);
        g.fillPath (headPath);

        // fur strokes, clipped to the head
        g.saveState();
        g.reduceClipRegion (headPath, juce::AffineTransform());
        juce::Random fr (99);
        for (int i = 0; i < 260; ++i)
        {
            const float an = fr.nextFloat() * pi * 2.0f;
            const float rr = std::sqrt (fr.nextFloat());
            const float x = cx + std::cos (an) * rr * R;
            const float y = cy + std::sin (an) * rr * R * 0.92f;
            const float d = an + (fr.nextFloat() - 0.5f) * 0.7f;
            const float l = 6.0f + fr.nextFloat() * 8.0f;
            const bool light = fr.nextFloat() < 0.5f;
            g.setColour (light ? juce::Colours::white.withAlpha (0.10f + 0.15f * v)
                               : juce::Colours::black.withAlpha (0.06f + 0.12f * v));
            g.drawLine (x, y, x + std::cos (d) * l, y + std::sin (d) * l, 1.0f);
        }
        // muzzle highlight for depth
        juce::ColourGradient mg (mixC (0xffffffff, 0xffc9c9cf, v).withAlpha (0.55f), cx, cy + R * 0.33f,
                                 juce::Colours::white.withAlpha (0.0f), cx + R * 0.5f, cy + R * 0.33f, true);
        g.setGradientFill (mg);
        g.fillEllipse (cx - R * 0.5f, cy + R * 0.33f - R * 0.4f, R, R * 0.8f);
        g.restoreState();

        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.strokePath (headPath, juce::PathStrokeType (2.0f));

        // red rim light from behind (lower right)
        if (v > 0.05f)
        {
            juce::Path rim;
            rim.addCentredArc (cx, cy, R * 0.985f, R * 0.905f, 0.0f, pi * 0.52f, pi * 1.35f, true);
            juce::DropShadow rg (juce::Colour (0xffff2a1a).withAlpha (0.8f * v), 14, { 0, 0 });
            rg.drawForPath (g, rim);
            g.setColour (juce::Colour (0xffff4030).withAlpha (0.55f * v));
            g.strokePath (rim, juce::PathStrokeType (5.0f));
        }
    }

    // ---- Beard ----
    if (kBeard > 0.01f)
    {
        juce::Path beard;
        beard.startNewSubPath (cx - R * 0.32f, cy + R * 0.62f);
        beard.quadraticTo (cx - R * 0.3f, cy + R * 0.9f, cx, cy + R * (0.8f + 0.25f * v));
        beard.quadraticTo (cx + R * 0.3f, cy + R * 0.9f, cx + R * 0.32f, cy + R * 0.62f);
        beard.closeSubPath();
        const auto bc = mixC (0xffffffff, 0xff8a8a90, v);
        g.setGradientFill (juce::ColourGradient (tint (bc, 1.1f).withAlpha (kBeard), cx, cy + R * 0.62f,
                                                 tint (bc, 0.45f).withAlpha (kBeard), cx, cy + R * (0.8f + 0.25f * v), false));
        g.fillPath (beard);
        g.setColour (juce::Colours::black.withAlpha (0.35f * kBeard));
        for (int i = -4; i <= 4; ++i)
            g.drawLine (cx + (float) i * R * 0.06f, cy + R * 0.68f,
                        cx + (float) i * R * 0.035f, cy + R * (0.72f + 0.2f * (1.0f - std::abs ((float) i) / 5.0f) + 0.1f * v), 1.2f);
    }

    // ---- Eye patches, glossy eyes, brows, blood tears ----
    for (int side = -1; side <= 1; side += 2)
    {
        const float s = (float) side;
        const float ex = cx + s * R * 0.42f;
        const float ey = cy - R * 0.05f;
        const float px = R * 0.2f * (1.0f + 0.25f * v);
        const float py = R * 0.28f * (1.0f + 0.2f * v);

        juce::Path patch;
        patch.addEllipse (ex - px, ey - py, px * 2.0f, py * 2.0f);
        patch.applyTransform (juce::AffineTransform::rotation (-s * (0.45f + 0.15f * v), ex, ey));
        softShadow (g, patch, juce::Colours::black.withAlpha (0.7f), 8, 0, 0);
        juce::ColourGradient pg (juce::Colour (0xff3c3c42), ex - px * 0.3f, ey - py * 0.4f,
                                 juce::Colour (0xff030303), ex - px * 0.3f + py * 1.2f, ey - py * 0.4f, true);
        pg.addColour (0.5, juce::Colour (0xff151517));
        g.setGradientFill (pg);
        g.fillPath (patch);

        const float er = R * 0.1f;
        fillSphere (g, ex, ey, er, er, mixC (0xffffffff, 0xff0c0c0c, v), 1.5f, 0.3f);
        if (v > 0.2f)
        {
            g.setColour (juce::Colour (0xffd8d8d8).withAlpha (v * 0.5f));
            g.drawEllipse (ex - er * 1.2f, ey - er * 1.2f, er * 2.4f, er * 2.4f, 2.0f);
        }

        const float pw = R * 0.06f * (1.0f - 0.7f * v);
        const float ph = R * 0.09f;
        if (v > 0.1f)
            softShadow (g, ellipsePath (ex, ey, pw, ph), juce::Colour (0xffff2020).withAlpha (v), 10, 0, 0);
        g.setColour (mixC (0xff000000, 0xffd02020, v));
        g.fillEllipse (ex - pw, ey - ph, pw * 2.0f, ph * 2.0f);

        // wet specular highlights
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillEllipse (ex - er * 0.38f - er * 0.28f, ey - er * 0.42f - er * 0.2f, er * 0.56f, er * 0.4f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.fillEllipse (ex + er * 0.32f - er * 0.1f, ey + er * 0.38f - er * 0.1f, er * 0.2f, er * 0.2f);

        // brows
        const float half = R * 0.2f;
        const float browY = ey - R * 0.3f;
        const float outerY = browY - R * 0.08f * v;
        const float innerY = browY + R * (0.15f * v - 0.06f * (1.0f - v));
        juce::Path brow;
        brow.startNewSubPath (ex + s * half, outerY);
        brow.lineTo (ex - s * half, innerY);
        softShadow (g, brow, juce::Colours::black.withAlpha (0.5f), 5, 0, 3);
        g.setColour (ink);
        g.drawLine (ex + s * half, outerY, ex - s * half, innerY, R * 0.07f);

        // blood tears
        if (kTears > 0.01f)
        {
            for (int i = 0; i < 2; ++i)
            {
                const float x0 = ex + s * (R * 0.02f + (float) i * R * 0.07f);
                const float y0 = ey + er * 0.8f;
                const float len = R * (0.55f + 0.15f * (float) i) * kTears
                                + std::sin (t * 1.5f + (float) i) * 3.0f * kTears;
                juce::Path p, hl;
                p.startNewSubPath (x0, y0);
                hl.startNewSubPath (x0 - 1.0f, y0);
                for (float yy = 0.0f; yy <= len; yy += 4.0f)
                {
                    const float dx = std::sin (yy * 0.08f + (float) i) * 2.0f;
                    p.lineTo (x0 + dx, y0 + yy);
                    hl.lineTo (x0 - 1.0f + dx, y0 + yy);
                }
                g.setColour (juce::Colour (0xff4a0606).withAlpha (kTears));
                g.strokePath (p, juce::PathStrokeType (3.5f - (float) i * 0.8f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
                g.setColour (juce::Colour (0xffff8c8c).withAlpha (0.35f * kTears));
                g.strokePath (hl, juce::PathStrokeType (1.0f));
                g.setColour (juce::Colour (0xff4a0606).withAlpha (kTears));
                g.fillEllipse (x0 - 3.0f, y0 + len - 2.0f, 6.0f, 8.0f);
                g.setColour (juce::Colour (0xffffc8c8).withAlpha (0.6f * kTears));
                g.fillEllipse (x0 - 1.9f, y0 + len - 0.4f, 1.8f, 2.8f);
            }
        }
    }

    // ---- Faceted metal crown ----
    if (kCrown > 0.01f)
    {
        // shadow the crown casts on the forehead
        juce::ColourGradient fs (juce::Colours::black.withAlpha (0.45f * kCrown), cx, cy - R * 0.45f,
                                 juce::Colours::black.withAlpha (0.0f), cx + R * 0.8f, cy - R * 0.45f, true);
        g.setGradientFill (fs);
        g.fillEllipse (cx - R * 0.7f, cy - R * 0.42f - R * 0.22f, R * 1.4f, R * 0.44f);

        juce::Path band;
        band.startNewSubPath (cx - R * 0.62f, cy - R * 0.45f);
        band.quadraticTo (cx, cy - R * 0.75f, cx + R * 0.62f, cy - R * 0.45f);
        band.lineTo (cx + R * 0.62f, cy - R * 0.58f);
        band.quadraticTo (cx, cy - R * 0.88f, cx - R * 0.62f, cy - R * 0.58f);
        band.closeSubPath();
        softShadow (g, band, juce::Colours::black.withAlpha (0.6f * kCrown), 10, 0, 5);
        juce::ColourGradient bg (juce::Colour (0xff7a7a88).withAlpha (kCrown), 0.0f, cy - R * 0.88f,
                                 juce::Colour (0xff06060a).withAlpha (kCrown), 0.0f, cy - R * 0.45f, false);
        bg.addColour (0.3, juce::Colour (0xff26262e).withAlpha (kCrown));
        g.setGradientFill (bg);
        g.fillPath (band);

        juce::Path topEdge;
        topEdge.startNewSubPath (cx - R * 0.62f, cy - R * 0.58f);
        topEdge.quadraticTo (cx, cy - R * 0.88f, cx + R * 0.62f, cy - R * 0.58f);
        g.setColour (juce::Colour (0xffb4b4c4).withAlpha (kCrown));
        g.strokePath (topEdge, juce::PathStrokeType (1.5f));

        const float xo[5] = { -0.5f, -0.25f, 0.0f, 0.25f, 0.5f };
        const float hh[5] = { 0.5f, 0.7f, 0.95f, 0.7f, 0.5f };
        for (int i = 0; i < 5; ++i)
        {
            const float tt = (xo[i] + 0.62f) / 1.24f;
            const float by = cy + R * (-0.58f - 0.6f * tt * (1.0f - tt));
            const float x = cx + xo[i] * R;
            const float tipx = x + xo[i] * R * 0.25f;
            const float tipy = by - hh[i] * R * kCrown;
            const float bl = x - R * 0.06f, br = x + R * 0.06f;

            faceTri (g, bl, by + 2.0f, tipx, tipy, x, by + 2.0f,
                     juce::Colour (0xff52525e).withAlpha (kCrown), bl, by,
                     juce::Colour (0xffd0d0de).withAlpha (kCrown), tipx, tipy);
            faceTri (g, x, by + 2.0f, tipx, tipy, br, by + 2.0f,
                     juce::Colour (0xff0c0c12).withAlpha (kCrown), x, by,
                     juce::Colour (0xff2c2c36).withAlpha (kCrown), tipx, tipy);
        }

        const float vy = cy - R * 0.5f + R * 0.28f * kCrown;
        juce::Path vl, vr;
        vl.addTriangle (cx - R * 0.1f, cy - R * 0.5f, cx, vy, cx, cy - R * 0.5f);
        vr.addTriangle (cx, cy - R * 0.5f, cx, vy, cx + R * 0.1f, cy - R * 0.5f);
        g.setColour (juce::Colour (0xff6a6a78).withAlpha (kCrown));
        g.fillPath (vl);
        g.setColour (juce::Colour (0xff0c0c12).withAlpha (kCrown));
        g.fillPath (vr);
    }

    // ---- Cheeks (blush fades) ----
    g.setColour (juce::Colour (0xffff8fb0).withAlpha ((1.0f - v) * 0.5f));
    g.fillEllipse (cx - R * 0.62f - R * 0.11f, cy + R * 0.36f - R * 0.07f, R * 0.22f, R * 0.14f);
    g.fillEllipse (cx + R * 0.62f - R * 0.11f, cy + R * 0.36f - R * 0.07f, R * 0.22f, R * 0.14f);

    // ---- Glossy nose ----
    fillSphere (g, cx, cy + R * 0.2f, R * 0.12f, R * 0.08f, juce::Colour (0xff1a1a1a), 2.2f, 0.4f);
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.fillEllipse (cx - R * 0.04f - R * 0.035f, cy + R * 0.17f - R * 0.02f, R * 0.07f, R * 0.04f);

    // ---- Mouth: bloody, shaded fangs, drips ----
    {
        const float my = cy + R * 0.42f;
        const float mw = R * (0.22f + 0.3f * v);
        const float depth = R * (0.12f + 0.35f * v);

        juce::Path mouth;
        mouth.startNewSubPath (cx - mw, my);
        mouth.quadraticTo (cx, my + depth * 2.0f, cx + mw, my);

        if (v > 0.3f)
        {
            const float al = juce::jlimit (0.0f, 1.0f, (v - 0.3f) * 3.0f);
            juce::Path filled (mouth);
            filled.closeSubPath();
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a0004).withAlpha (al), 0.0f, my,
                                                     juce::Colour (0xff6a0a10).withAlpha (al), 0.0f, my + depth, false));
            g.fillPath (filled);

            for (int side = -1; side <= 1; side += 2)
            {
                const float fx = cx + (float) side * mw * 0.55f;
                const float fl = R * 0.14f * v;
                juce::Path lf, rf;
                lf.addTriangle (fx - R * 0.04f, my, fx, my + fl, fx, my);
                rf.addTriangle (fx, my, fx, my + fl, fx + R * 0.04f, my);
                g.setColour (juce::Colour (0xfffffdf2));
                g.fillPath (lf);
                g.setColour (juce::Colour (0xffb8b09a));
                g.fillPath (rf);
            }
        }

        g.setColour (ink);
        g.strokePath (mouth, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        if (kTears > 0.01f)
        {
            const float fs[3] = { -0.5f, 0.0f, 0.45f };
            for (int i = 0; i < 3; ++i)
            {
                const float tt = 0.5f + fs[i] * 0.5f;
                const float x0 = cx + fs[i] * mw;
                const float y0 = my + 4.0f * depth * tt * (1.0f - tt);
                const float L = R * (0.2f + 0.12f * (float) i) * kTears + std::sin (t * 1.3f + (float) i) * 2.0f * kTears;

                juce::ColourGradient dg (juce::Colour (0xff2a0404).withAlpha (kTears), x0 - 2.0f, 0.0f,
                                         juce::Colour (0xff3a0606).withAlpha (kTears), x0 + 2.0f, 0.0f, false);
                dg.addColour (0.4, juce::Colour (0xff9a2222).withAlpha (kTears));
                g.setGradientFill (dg);
                g.fillRect (x0 - 2.0f, y0, 4.0f, L);
                g.fillEllipse (x0 - 3.2f, y0 + L - 2.5f, 6.4f, 9.0f);
                g.setColour (juce::Colour (0xffffc8c8).withAlpha (0.6f * kTears));
                g.fillEllipse (x0 - 1.7f, y0 + L - 0.4f, 1.8f, 2.8f);
            }
        }
    }

    // ---- Title: extruded 3D BASSFACE at full rage ----
    if (v >= 0.85f)
    {
        const juce::String T ("BASSFACE");
        g.setFont (juce::Font (juce::FontOptions (46.0f, juce::Font::bold)));
        auto jit = [&] { return (rng.nextFloat() - 0.5f) * 1.2f; };

        {
            juce::Path glowShape;
            glowShape.addRectangle (w * 0.5f - 130.0f, 18.0f, 260.0f, 36.0f);
            juce::DropShadow gl (juce::Colour (0xffff1e14).withAlpha (0.6f), 24, { 0, 0 });
            gl.drawForPath (g, glowShape);
        }

        for (int i = 9; i >= 1; --i)
        {
            g.setColour (mixC (0xff8a0c0c, 0xff240000, (float) i / 9.0f));
            g.drawText (T, (int) ((float) i * 0.9f), 14 + (int) ((float) i * 0.9f), getWidth(), 56,
                        juce::Justification::centredTop);
        }

        g.setColour (juce::Colour (0xff1a0000));
        for (int ox = -2; ox <= 2; ox += 2)
            for (int oy = -2; oy <= 2; oy += 2)
                g.drawText (T, ox, 14 + oy, getWidth(), 56, juce::Justification::centredTop);

        g.setColour (juce::Colour (0xffe9e4d4));
        g.drawText (T, (int) jit(), 14, getWidth(), 56, juce::Justification::centredTop);
    }
    else
    {
        g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
        g.setColour (mixC (0xff2b2b2b, 0xffe9e4d4, v));
        g.drawText (v < 0.35f ? "PANDA CRUNCH" : "PANDA RAGE",
                    0, 12, getWidth(), 40, juce::Justification::centred);
    }

    // ---- Knob housing (the knob itself is drawn by PandaKnobLook) ----
    {
        const float kx = w * 0.5f, ky = h - 115.0f;
        const auto pb = mixC (0xff3a3a3a, 0xff0c0000, v);
        softShadow (g, ellipsePath (kx, ky, 88.0f, 88.0f), juce::Colours::black.withAlpha (0.6f), 20, 0, 10);
        g.setGradientFill (juce::ColourGradient (tint (pb, 1.9f), kx - 88.0f, ky - 88.0f,
                                                 tint (pb, 0.4f), kx + 88.0f, ky + 88.0f, false));
        g.fillEllipse (kx - 86.0f, ky - 86.0f, 172.0f, 172.0f);
        g.setColour (mixC (0xff141418, 0xff060004, v));
        g.fillEllipse (kx - 80.0f, ky - 80.0f, 160.0f, 160.0f);

        g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
        g.setColour (mixC (0xff2b2b2b, 0xffc43a3a, v));
        g.drawText ("RAGE", 0, getHeight() - 32, getWidth(), 22, juce::Justification::centred);
    }

    // ---- Vignette + noise takeover ----
    if (v > 0.05f)
    {
        juce::ColourGradient vg (juce::Colours::black.withAlpha (0.0f), w * 0.5f, h * 0.5f,
                                 juce::Colours::black.withAlpha (0.7f * v), w * 0.5f + h * 0.75f, h * 0.5f, true);
        vg.addColour (0.33, juce::Colours::black.withAlpha (0.0f));
        g.setGradientFill (vg);
        g.fillRect (getLocalBounds());

        const int n = (int) (v * v * 2500.0f);
        for (int i = 0; i < n; ++i)
        {
            const float l = rng.nextFloat();
            g.setColour (juce::Colour::fromFloatRGBA (l, l, l, 0.2f + 0.6f * rng.nextFloat()));
            const float s = 1.0f + rng.nextFloat() * 2.0f;
            g.fillRect (rng.nextFloat() * w, rng.nextFloat() * h, s, s);
        }

        if (v > 0.6f && rng.nextFloat() < 0.3f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.15f * v));
            g.fillRect (0.0f, rng.nextFloat() * h, w, 4.0f + rng.nextFloat() * 16.0f);
        }
    }
}
