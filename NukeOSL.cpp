/*
/Users/jermainedupris/repos/VFX/OpenShadingLanguage/tmpbuild/src/shaders/ubersurface.oso

    NukeOSL.cpp: dynamic knob demo plugin

    Copyright (C) 2010  Jonathan Egstad

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <DDImage/Iop.h>
#include <DDImage/MultiTileIop.h>
#include <DDImage/Tile.h>
#include <DDImage/Row.h>
#include <DDImage/Knobs.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <OSL/oslquery.h>
#pragma clang diagnostic pop

#include <sstream>

using namespace DD::Image;
using namespace OSL;

//-----------------------------------------------------------------
//-----------------------------------------------------------------

// Super-simple knob reference object.
class DynamicKnob {
    friend class DynamicKnobManager;
    enum Type {
        Invalid     = 0,
        Integral    = 1,
        Floating    = 2,
        String      = 3,
        ScalarTypes = 0x7,

        Vector2  = Floating | 8,
        Vector3  = Floating | 16,
        Vector4  = Floating | 32,
        Color3   = Floating | 64,
        Color4   = Floating | 128,

        Menu     = Integral | 8,
        Bool     = Integral | 16,

        File       = String | 8,
        TabGroup   = String | 16,
        TabBegin   = String | 32,
        GroupBegin = String | 64,
        GroupEnd   = String | 128,
    };

    std::string mName = "";       //!< Attribute name
    std::string mHelp = "";       //!< Attribute help / tooltip
    Knob*       mKnob = nullptr;  //!< The current Nuke knob this drives
    uint8_t     mType = Invalid;  //!< Attribute type
    std::vector<char> mMenuEntries;

    template <typename T> struct Value {
        std::vector<T> values;
        std::vector<T> defaults;

        Value(size_t n, T def = {}) : values(n, def), defaults(n, def) {}
        T* data(bool create) { return create ? defaults.data() : values.data(); }
        size_t size() const { return std::min(values.size(), defaults.size()); }
        bool operator == (const Value& v) const {
            size_t nVals = defaults.size();
            if (nVals != v.defaults.size())
                return false;
            for (size_t i = 0; i < nVals; ++i) {
                if (defaults[i] != v.defaults[i])
                    return false;
            }
            return true;
        }
    };
    template <typename T> using ValuePtr = std::unique_ptr<Value<T>>;
    struct {
        ValuePtr<int>         ints;
        ValuePtr<float>       floats;
        ValuePtr<const char*> strings;
    } mValues;

    void swap(DynamicKnob&& other) {
        mName.swap(other.mName);
        mHelp.swap(other.mHelp);
        std::swap(mKnob, other.mKnob);
        std::swap(mType, other.mType);
        mMenuEntries.swap(other.mMenuEntries);
        mValues.ints.swap(other.mValues.ints);
        mValues.floats.swap(other.mValues.floats);
        mValues.strings.swap(other.mValues.strings);
    }

    size_t numValues() const {
        switch (mType & ScalarTypes)
        {
            case Integral: return mValues.ints->size();
            case Floating: return mValues.floats->size();
            case String:   return mValues.strings->size();
        }
        return 0UL;
    }
    
    template <typename Params>
    void setValue(unsigned i, const Params& params) {
        switch (mType & ScalarTypes) {
            case Integral: {
                const int ival = params.defaults(i, 0);
                mValues.ints->values[i] = ival;
                mValues.ints->defaults[i] = ival;
            }
            break;

            case Floating: {
                const float fval = params.defaults(i, 0.f);
                mValues.floats->values[i] = fval;
                mValues.floats->defaults[i] = fval;
            }
            break;

            case String: {
                const char* sval = params.defaults(i, "");
                mValues.strings->values[i] = sval;
                mValues.strings->defaults[i] = sval;
            }
            break;

            default:
                assert(0 && "Unreachable");
        };
    }

    Knob* makeKnob(Knob_Callback f, bool create) {
        auto makeNukeKnob = [&]() -> Knob* {
            switch (mType)
            {
                case Bool:     //return f(BOOL_KNOB, IntPtr, mValues.ints->data(create), mName.c_str());
                case Integral: return MultiInt_knob    (f, mValues.ints->data(create), mValues.ints->size(), mName.c_str());
                case Menu:     return Enumeration_knob (f, mValues.ints->data(create), &mValues.strings->defaults[0], mName.c_str());

                case File:     //return File_knob(f, mValues.strings->data(create), mName.c_str());
                case String:      return String_knob  (f, mValues.strings->data(create), mName.c_str());
                case TabGroup:    return BeginTabGroup(f, mName.c_str());
                case TabBegin:    return Tab_knob(f, mName.c_str());
                case GroupBegin:  return BeginGroup(f, mName.c_str());
                case GroupEnd:    return EndGroup(f);

                case Vector4:
                case Floating: return MultiFloat_knob  (f, mValues.floats->data(create), mValues.floats->size(), mName.c_str());
                case Vector2:  return XY_knob      (f, mValues.floats->data(create), mName.c_str());
                case Vector3:  return XYZ_knob     (f, mValues.floats->data(create), mName.c_str());
                case Color3:   return Color_knob   (f, mValues.floats->data(create), mName.c_str());
                case Color4:   return AColor_knob  (f, mValues.floats->data(create), mName.c_str());
            }
            return nullptr;
        };
        Knob* k = makeNukeKnob();
        if (!k)
            return nullptr;
#if 0
        if (0) {
            // SetRange(f, double minimum, double maximum);
            // if (!uiRange) k->set_flag(Knob::FORCE_RANGE);
        }
#endif
        if (!mHelp.empty())
            Tooltip(f, mHelp);

        return k;
    }
    
    void print() const {
        std::cout << mType << ":'" << mName << "' [";
        for (unsigned j = 0, n = numValues(); j < n; ++j) {
            std::cout << " ";
            switch (mType & ScalarTypes) {
                case Integral:  std::cout << mValues.ints->values[j]; break;
                case Floating:  std::cout << mValues.floats->values[j]; break;
                case String:    std::cout << mValues.strings->values[j]; break;
                default:
                    assert(0 && "Unreachable");
            };
        }
        std::cout << "]" << std::endl;
    }

public:
    DynamicKnob() {}

    DynamicKnob(std::string name, uint8_t type)
        : mName(std::move(name))
        , mType(type) {
    }

    template <typename Parser>
    DynamicKnob(std::string name, uint8_t type, size_t nValues,
                Parser& params = {})
        : mName(std::move(name))
        , mType(type)
        , mHelp(std::move(params.mHelp))
    {
        switch (mType & ScalarTypes)
        {
            case Integral:
                mValues.ints.reset(new Value<int>(nValues));
                if (params.mMenuEntries) {
                    mValues.strings.reset(new Value<const char*>(0));
                    mMenuEntries.assign(params.mMenuEntries->begin(), params.mMenuEntries->end());
                    mMenuEntries.emplace_back(0);
                    const char* entry = strtok(mMenuEntries.data(), "|");
                    do {
                        mValues.strings->defaults.emplace_back(entry);
                        entry = strtok(NULL, "|");
                    } while (mValues.strings->defaults.back());
                }
                break;
            case Floating: mValues.floats.reset(new Value<float>(nValues)); break;
            case String:   mValues.strings.reset(new Value<const char*>(nValues)); break;
        }

        assert(numValues() == nValues && "Size mis-match");
        for (size_t i = 0; i < nValues; ++i)
            setValue(i, params);
    }

    DynamicKnob(DynamicKnob&& other) { swap(std::move(other)); }

    void operator () (Knob_Callback f, bool create)
    {
        // If the knob's being (re)created, we need to point to the default value:
        Knob* k  = makeKnob(f, create);
        if (k && create) {
            size_t i = 0;
            switch (mType)
            {
                case Integral:
                    for (auto&& val : mValues.ints->values)
                        k->set_value(val, i++);
                break;

                case Floating:
                    for (auto&& val : mValues.floats->values)
                        k->set_value(val, i++);
                break;

                case String: {
                    std::string out;
                    for (auto&& val : mValues.strings->values)
                        out += val;
                    k->set_text(out.c_str());
                }
                break;
            }
            k->changed();
            // knob = k;
        }
    }
    
    bool operator == (const DynamicKnob& other) {
        if (mType != other.mType)
            return false;
        if (mName != other.mName)
            return false;
        switch (mType & ScalarTypes)
        {
            case Integral:
                if (!mValues.ints)
                    return !other.mValues.ints;
                return *mValues.ints == *other.mValues.ints;
            case Floating:
                if (!mValues.floats)
                    return !other.mValues.floats;
                return *mValues.floats == *other.mValues.floats;
            case String:
                if (!mValues.strings)
                    return !other.mValues.strings;
                return *mValues.strings == *other.mValues.strings;
        }
        return false;
    }

    bool isInput() const {
        if (mType == File)
            return true;
        if (mName == "filename")
            return true;
        return false;
    }
};

typedef std::vector<DynamicKnob> DynamicKnobList;

//-----------------------------------------------------------------

class DynamicKnobManager {
    using FillKnobList = bool (*) (const char* src, DynamicKnobList& out_list, Op*);

    Op*             mParent;            //!< Owner
    FillKnobList    mFillCallback;
    DynamicKnobList mKnobs;             //!< The list of dynamic knobs
    Knob*           mPutKnobsAfterThis; //!< Remember where the knobs are being inserted
    int             mNumKobsAdded;      //!< How many added knobs there currently are
    unsigned        mBuildNum;          //!< How many times have the knobs been built

    std::vector<std::string> mInputs;

    /*! Add the dynamic knobs in 'create' mode */
    static void createKnobs(void* p, Knob_Callback f)
    {
        if (p)
            reinterpret_cast<DynamicKnobManager*>(p)->addKnobs(f, true /*create_knobs*/);
    }

    /*! Add the dynamic knobs in 'refresh' mode */
    static void refreshKnobs(void* p, Knob_Callback f)
    {
        if (p)
            reinterpret_cast<DynamicKnobManager*>(p)->addKnobs(f, false /*create_knobs*/);
    }

    static bool makeOSLParameters(const char* shadername,
                                  DynamicKnobList& out_list,
                                  Op* errors);

public:

    DynamicKnobManager(Op* parent, FillKnobList cb = makeOSLParameters)
        : mParent(parent)
        , mFillCallback(cb)
        , mNumKobsAdded(0)
        , mBuildNum(0)
    {
    }

    /*! How many times have the knobs been built? */
    unsigned buildNum() const { return mBuildNum; }

    /*! Create/refresh the dynamic knobs.  If they're being re-created we need to
     establish
     the knobs with the default value so that any subsequent value change will
     be saved
     to a script.  After creation we set them back to the current values.
  */
    void addKnobs(Knob_Callback f, bool create)
    {
        mInputs.clear();
    
        KnobChangeGroup knobChanged;
        bool tabs = false;
        for (auto&& dKnob : mKnobs) {
            tabs = tabs || dKnob.mType == DynamicKnob::TabGroup;
            dKnob(f, create);
            if (dKnob.isInput())
                mInputs.emplace_back(dKnob.mName);
        }
        if (tabs)
            EndTabGroup(f);
    }

    /*! */
    void knobs(Knob_Callback f, const char* src)
    {
        if (f.makeKnobs()) {
            // Only do this when the knobs are first created.
            // At this point the contents of whatever knob is used to construct the
            // knob list is at its default value, *not* the one loaded from a script:
            mPutKnobsAfterThis = f.getLastMadeKnob();

            if (mFillCallback)
                (*mFillCallback)(src, mKnobs, mParent);
            mNumKobsAdded = mParent->Op::add_knobs(createKnobs, this, f);
        } else if (mNumKobsAdded > 0) {
            mParent->Op::add_knobs(refreshKnobs, this, f);
        } else if (mKnobs.size() > 0) {
            addKnobs(f, false /*create_knobs*/);
        }
    }

    /*!  */
    void rebuildKnobs(const char* src)
    {
        ++mBuildNum;
        if (mFillCallback && (*mFillCallback)(src, mKnobs, mParent)) {
            mNumKobsAdded = mParent->Op::replace_knobs(mPutKnobsAfterThis, mNumKobsAdded,
                                                       createKnobs, this);
        }
    }

    const std::vector<std::string>& inputs() const { return mInputs; }
};

struct OSLValues {
    const OSLQuery::Parameter& mParameter;
    std::string mHelp = {};
    const OIIO::ustring* mMenuEntries = nullptr;

    OSLValues(const OSLQuery::Parameter& p) : mParameter(p) {}

    float defaults(unsigned i, float def) const {
        return i < mParameter.fdefault.size() ? mParameter.fdefault[i] : def;
    }
    int defaults(unsigned i, int def) const {
        return i < mParameter.idefault.size() ? mParameter.idefault[i] : def;
    }
    const char* defaults(unsigned i, const char* def) const {
        return i < mParameter.sdefault.size() ? mParameter.sdefault[i].c_str() : def;
    }

    const OSLQuery::Parameter* operator -> () const { return &mParameter; }
};

bool
DynamicKnobManager::makeOSLParameters(const char* shadername,
                                      DynamicKnobList& out_list,
                                      Op* errors)
{
    OSLQuery osl;
    if (shadername && *shadername && !osl.open(shadername)) {
        errors->error(("OSLQuery error: " + osl.geterror()).c_str());
        return false;
    }

    DynamicKnobList newKnobs;
    std::map<std::string, DynamicKnobList> groups;
    std::vector<decltype(groups)::iterator> grouporder;
    for (size_t i = 0, n = osl.nparams(); i < n; ++i) {
        OSLValues oslParam(*osl.getparam(i));
        if (oslParam->isclosure) {
            errors->warning("Skipping parameter '%s': closures not supported.",
                            oslParam->name.c_str());
            continue;
        }
        if (oslParam->isstruct) {
            errors->warning("Skipping parameter '%s': struct '%s.' not supported.",
                            oslParam->name.c_str(), oslParam->structname.c_str());
            continue;
        }
        if (oslParam->type.arraylen < 0) {
                errors->warning("Skipping parameter '%s': varying arrays not supported.",
                                oslParam->name.c_str());
            continue;
        }
        if (oslParam->type.arraylen > 1 && oslParam->type.aggregate != OIIO::TypeDesc::SCALAR) {
            errors->warning("Skipping parameter '%s': arrays of aggregates not supported.",
                            oslParam->name.c_str());
            continue;
        }

        unsigned type = DynamicKnob::Invalid;
        int arraylen = oslParam->type.arraylen ? oslParam->type.arraylen : 1;
        switch (oslParam->type.basetype) {
            case OIIO::TypeDesc::INT8:
            case OIIO::TypeDesc::UINT8:
            case OIIO::TypeDesc::INT16:
            case OIIO::TypeDesc::UINT16:
            case OIIO::TypeDesc::INT32:
            case OIIO::TypeDesc::UINT32:
                type = DynamicKnob::Integral;
                break;

            case OIIO::TypeDesc::HALF:
            case OIIO::TypeDesc::FLOAT:
            case OIIO::TypeDesc::DOUBLE:
                type = DynamicKnob::Floating;
                switch (oslParam->type.vecsemantics) {
                    case OIIO::TypeDesc::VEC2:  type = DynamicKnob::Vector2; break;
                    case OIIO::TypeDesc::VEC3:  type = DynamicKnob::Vector3; break;
                    case OIIO::TypeDesc::VEC4:  type = DynamicKnob::Vector4; break;
                    case OIIO::TypeDesc::COLOR:
                        switch(oslParam->type.aggregate) {
                            case OIIO::TypeDesc::VEC3: type = DynamicKnob::Color3;  break;
                            case OIIO::TypeDesc::VEC4: type = DynamicKnob::Color4;  break;
                        }
                        break;
                    default: break;
                }
                break;
            case OIIO::TypeDesc::STRING:
                type = DynamicKnob::String;
                {
                    bool popup = false;
                    bool filepath = false;
                    for (auto&& md : oslParam->metadata) {
                        if (md.name == "widget" && !md.sdefault.empty()) {
                            popup = md.sdefault[0] == "popup";
                            filepath =  md.sdefault[0] == "filename";
                        }
                        else if (popup && md.name == "options" && !md.sdefault.empty()) {
                            oslParam.mMenuEntries = &md.sdefault[0];
                        }
                    }
                    if (popup && oslParam.mMenuEntries)
                        type = DynamicKnob::Menu;
                    else if (filepath)
                        type = DynamicKnob::File;
                }
                break;
            default:
                errors->warning("Skipping parameter '%s': unsupported type", oslParam->name.c_str());
                continue;
        }
        assert(type != DynamicKnob::Invalid && "Unkown type");

        DynamicKnobList* addTo = &newKnobs;
        for (auto&& md : oslParam->metadata) {
            if (md.name == "help" && !md.sdefault.empty()) {
                oslParam.mHelp = md.sdefault[0].string();
            } else if (md.name == "page" && !md.sdefault.empty()) {
                auto itr = groups.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(md.sdefault[0].string()),
                                   std::forward_as_tuple(0));
                addTo = &itr.first->second;
                if (itr.second)
                    grouporder.emplace_back(std::move(itr.first));
            }
            // min max UImin UImax widget = checkBox
        }

        addTo->emplace_back(oslParam->name.string(), type,
                            size_t(oslParam->type.aggregate * arraylen),
                            oslParam);
    }

    if (!grouporder.empty()) {
        newKnobs.emplace_back("", DynamicKnob::TabGroup);
        std::string lastGroup;
        size_t nGroups = 0;
        for (auto&& group : grouporder) {
            if (lastGroup.empty() || group->first.find(lastGroup) == std::string::npos) {
                newKnobs.emplace_back(group->first, DynamicKnob::TabBegin);
                lastGroup = group->first;
            } else
                newKnobs.emplace_back(group->first, DynamicKnob::GroupBegin);

            for (auto&& param : group->second)
                newKnobs.emplace_back(std::move(param));
        }
    }

    if (out_list.empty() != newKnobs.empty()) {
        out_list.swap(newKnobs);
        return true;
    }

    size_t same = 0;
    for (auto&& newKnob : newKnobs) {
        for (auto&& oldKnob : out_list) {
            if (newKnob == oldKnob) {
                newKnob.swap(std::move(oldKnob));
                ++same;
                break;
            }
        }
    }
    out_list.swap(newKnobs);
    return same != out_list.size();
}

//---------------------------------------------------------------------------------------

/*! Test op to hold dynamic knobs.  Does nothing.
*/
class NukeOSL : public Iop {
    Knob*              mUpdateKnob = nullptr;
    Knob*              mFileKnob = nullptr;
    const char*        mOSLFile = "";
    DynamicKnobManager mKnobManager;

public:
/*
    int maximum_inputs() const override {
        return mKnobManager.inputs().size();
    }

    bool test_input(int i, Op* op) const override {
        return dynamic_cast<Iop*>(input(i));
    }
*/
    Op* default_input(int i) const override
    {
        if (i == 0)
            return Iop::default_input(0);

        return nullptr;
    }

    bool test_input(int i, Op* op) const override
    {
        if (i==0)
            return Iop::test_input(i, op);

        return op == nullptr || dynamic_cast<Iop*>(op) != nullptr;
    }

    const char* input_label(int i, char* buf) const override
    {
        if (i == 0)
            return "";

        const auto& inputs = mKnobManager.inputs();
        return i <= inputs.size() ? inputs[i-1].c_str() : "unknown";
    }
  
    static const Description kDescription;
    const char* Class() const override { return kDescription.name; }
    const char* node_help() const override
    {
        return __DATE__ " " __TIME__
                        "\n"
                        "Dynamically change the knob set based on the contents of "
                        "the code text field.";
    }

    NukeOSL(Node* node)
        : Iop(node)
        , mKnobManager(this->firstOp())
    {
    }

    /*virtual*/
    void knobs(Knob_Callback f) override
    {
        mFileKnob = File_knob(f, &mOSLFile, "file");
        SetFlags(f, Knob::EARLY_STORE | Knob::KNOB_CHANGED_ALWAYS | Knob::NO_ANIMATION);
        mUpdateKnob = Script_knob(f, "", "update", "          UPDATE          ");
        SetFlags(f, Knob::HIDDEN);
        Divider(f);

        mKnobManager.knobs(f, mFileKnob->get_text());
        inputs(1 + mKnobManager.inputs().size());
    }

    /*virtual*/
    int knob_changed(Knob* k) override
    {
        if (k == mFileKnob || k == mUpdateKnob) {
            mKnobManager.rebuildKnobs(mFileKnob->get_text());
            return 1;
        }
        return 0;
    }

    void _validate(bool forReal) override
    {
        if (!forReal) {
            validate(true);
            return;
        }
        for (int i = 0, n = inputs();i < n; ++i) {
printf("input[%d] = %p\n", i, input(i));
        }

        //clear_info();
        copy_info();
        for (int i = 1, n = inputs();i < n; ++i) {
            if (Iop* op = input(i)) {
                op->validate(forReal);
                // merge_info(i, Mask_All);
            }
        }
    }

    void _request(int x, int y, int r, int t, ChannelMask channels, int count) override
    {
        for (int i = 0, n = inputs();i < n; ++i) {
            if (Iop* iop = input(i))
                iop->request(channels, count);
        }
        set_out_channels(Mask_All);
    }

    void engine(int Y, int X, int R, ChannelMask channels, Row& row) override
    {
        MultiTileIop miop;
        std::vector<std::unique_ptr<Tile>> tiles(inputs());
        for (int i = 0, n = tiles.size();i < n; ++i) {
            if (Iop* iop = input(i)) {
                tiles[i].reset(new Tile(*iop, iop->requestedBox(), channels));
            }
            if (aborted())
                return;
        }

        foreach (z, channels) {
            float* outptr = row.writable(z) + X;
            for (int x = X ; x < R; ++x) {
                float value = 0;
                Tile& tile = *tiles[0];
                if ( intersect(tile.channels(), z) ) {
                    value += tile[z][Y][x];
                }
                *outptr++ = value;
            }
            if (aborted())
                return;
        }
    }
};
/*
void Normalise::engine ( int y, int x, int r,
                              ChannelMask channels, Row& row )
{
  {
    Guard guard(_lock);
    if ( _firstTime ) {
      // do anaylsis.
      Format format = input0().format();

      // these useful format variables are used later
      const int fx = format.x();
      const int fy = format.y();
      const int fr = format.r();
      const int ft = format.t();

      const int height = ft - fy ;
      const int width = fr - fx ;

      ChannelSet readChannels = input0().info().channels();

      Interest interest( input0(), fx, fy, fr, ft, readChannels, true );
      interest.unlock();

      // fetch each row and find the highest number pixel
      _maxValue = 0;
      for ( int ry = fy; ry < ft; ry++) {
        progressFraction( ry, ft - fy );
        Row row( fx, fr );
        row.get( input0(), ry, fx, fr, readChannels );
        if ( aborted() )
          return;

        foreach( z, readChannels ) {
          const float *CUR = row[z] + fx;
          const float *END = row[z] + fr;
          while ( CUR < END ) {
            _maxValue = std::max( (float)*CUR, _maxValue );
            CUR++;
          }
        }
      }
      _firstTime = false;
    }
  } // end lock

  Row in( x,r);
  in.get( input0(), y, x, r, channels );
  if ( aborted() )
    return;

  foreach( z, channels ) {
    float *CUR = row.writable(z) + x;
    const float* inptr = in[z] + x;
    const float *END = row[z] + r;
    while ( CUR < END ) {
        *CUR++ = *inptr++ * ( 1. / _maxValue );
    }
  }
}
*/

static Op* build(Node* node) { return new NukeOSL(node); }
const Op::Description NukeOSL::kDescription("NukeOSL", build);
