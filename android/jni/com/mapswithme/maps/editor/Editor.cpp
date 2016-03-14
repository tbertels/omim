﻿#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/osm_editor.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"
#include "std/vector.hpp"

namespace
{
osm::EditableMapObject g_editableMapObject;

jclass g_featureCategoryClazz;
jmethodID g_featureCtor;

jobject ToJavaFeatureCategory(JNIEnv * env, osm::Category const & category)
{
  return env->NewObject(g_featureCategoryClazz, g_featureCtor, category.m_type, jni::TScopedLocalRef(env, jni::ToJavaString(env, category.m_name)).get());
}
}  // namespace

extern "C"
{
using osm::Editor;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeInit(JNIEnv * env, jclass)
{
  g_featureCategoryClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/editor/data/FeatureCategory");;
  // public FeatureCategory(int category, String name)
  g_featureCtor = jni::GetConstructorID(env, g_featureCategoryClazz, "(ILjava/lang/String;)V");
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetMetadata(JNIEnv * env, jclass, jint type)
{
  // TODO(yunikkk): Switch to osm::Props enum instead of metadata, and use separate getters instead a generic one.
  return jni::ToJavaString(env, g_editableMapObject.GetMetadata().Get(static_cast<feature::Metadata::EType>(type)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetMetadata(JNIEnv * env, jclass clazz, jint type, jstring value)
{
  // TODO(yunikkk): I would recommend to use separate setters/getters for each metadata field.
  string const v = jni::ToNativeString(env, value);
  using feature::Metadata;
  switch (type)
  {
  // TODO(yunikkk): Pass cuisines in a separate setter via vector of osm untranslated keys (SetCuisines()).
  case Metadata::FMD_CUISINE: g_editableMapObject.SetRawOSMCuisines(v); break;
  case Metadata::FMD_OPEN_HOURS: g_editableMapObject.SetOpeningHours(v); break;
  case Metadata::FMD_PHONE_NUMBER: g_editableMapObject.SetPhone(v); break;
  case Metadata::FMD_FAX_NUMBER: g_editableMapObject.SetFax(v); break;
  case Metadata::FMD_STARS:
    {
      // TODO(yunikkk): Pass stars in a separate integer setter.
      int stars;
      if (strings::to_int(v, stars))
        g_editableMapObject.SetStars(stars);
      break;
    }
  case Metadata::FMD_OPERATOR: g_editableMapObject.SetOperator(v); break;
  case Metadata::FMD_URL:  // We don't allow url in UI. Website should be used instead.
  case Metadata::FMD_WEBSITE: g_editableMapObject.SetWebsite(v); break;
  case Metadata::FMD_INTERNET: // TODO(yunikkk): use separate setter for Internet.
    {
      osm::Internet inet = osm::Internet::Unknown;
      if (v == DebugPrint(osm::Internet::Wlan))
        inet = osm::Internet::Wlan;
      if (v == DebugPrint(osm::Internet::Wired))
        inet = osm::Internet::Wired;
      if (v == DebugPrint(osm::Internet::No))
        inet = osm::Internet::No;
      if (v == DebugPrint(osm::Internet::Yes))
        inet = osm::Internet::Yes;
      g_editableMapObject.SetInternet(inet);
    }
    break;
  case Metadata::FMD_ELE:
    {
      double ele;
      if (strings::to_double(v, ele))
        g_editableMapObject.SetElevation(ele);
      break;
    }
  case Metadata::FMD_EMAIL: g_editableMapObject.SetEmail(v); break;
  case Metadata::FMD_POSTCODE: g_editableMapObject.SetPostcode(v); break;
  case Metadata::FMD_WIKIPEDIA: g_editableMapObject.SetWikipedia(v); break;
  case Metadata::FMD_FLATS: g_editableMapObject.SetFlats(v); break;
  case Metadata::FMD_BUILDING_LEVELS: g_editableMapObject.SetBuildingLevels(v); break;
  case Metadata::FMD_TURN_LANES:
  case Metadata::FMD_TURN_LANES_FORWARD:
  case Metadata::FMD_TURN_LANES_BACKWARD:
  case Metadata::FMD_MAXSPEED:
  case Metadata::FMD_HEIGHT:
  case Metadata::FMD_MIN_HEIGHT:
  case Metadata::FMD_DENOMINATION:
  case Metadata::FMD_TEST_ID:
  case Metadata::FMD_COUNT:
    break;
  }
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSaveEditedFeature(JNIEnv *, jclass)
{
  switch (g_framework->NativeFramework()->SaveEditedMapObject(g_editableMapObject))
  {
  case osm::Editor::NothingWasChanged:
  case osm::Editor::SavedSuccessfully:
    return true;
  case osm::Editor::NoFreeSpaceError:
    return false;
  }
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsFeatureEditable(JNIEnv *, jclass)
{
  return g_framework->GetPlacePageInfo().IsEditable();
}

JNIEXPORT jintArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetEditableFields(JNIEnv * env, jclass clazz)
{
  auto const & editable = g_editableMapObject.GetEditableFields();
  int const size = editable.size();
  jintArray jEditableFields = env->NewIntArray(size);
  jint * arr = env->GetIntArrayElements(jEditableFields, 0);
  for (int i = 0; i < size; ++i)
    arr[i] = editable[i];
  env->ReleaseIntArrayElements(jEditableFields, arr, 0);

  return jEditableFields;
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsAddressEditable(JNIEnv * env, jclass clazz)
{
  return g_editableMapObject.IsAddressEditable();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsNameEditable(JNIEnv * env, jclass clazz)
{
  return g_editableMapObject.IsNameEditable();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetDefaultName(JNIEnv * env, jclass)
{
  // TODO(yunikkk): add multilanguage names support via EditableMapObject::GetLocalizedName().
  return jni::ToJavaString(env, g_editableMapObject.GetDefaultName());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetDefaultName(JNIEnv * env, jclass, jstring name)
{
  StringUtf8Multilang names = g_editableMapObject.GetName();
  // TODO(yunikkk): add multilanguage names support.
  names.AddString(StringUtf8Multilang::kDefaultCode, jni::ToNativeString(env, name));
  g_editableMapObject.SetName(names);
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetStreet(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetStreet());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetStreet(JNIEnv * env, jclass, jstring street)
{
  g_editableMapObject.SetStreet(jni::ToNativeString(env, street));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetHouseNumber(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetHouseNumber());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetHouseNumber(JNIEnv * env, jclass, jstring houseNumber)
{
  g_editableMapObject.SetHouseNumber(jni::ToNativeString(env, houseNumber));
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetNearbyStreets(JNIEnv * env, jclass clazz)
{
  auto const & streets = g_editableMapObject.GetNearbyStreets();
  int const size = streets.size();
  jobjectArray jStreets = env->NewObjectArray(size, jni::GetStringClass(env), 0);
  for (int i = 0; i < size; ++i)
    env->SetObjectArrayElement(jStreets, i, jni::TScopedLocalRef(env, jni::ToJavaString(env, streets[i])).get());
  return jStreets;
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeHasWifi(JNIEnv *, jclass)
{
  // TODO(AlexZ): Support 3-state: yes, no, unknown.
  return g_editableMapObject.GetMetadata().Get(feature::Metadata::FMD_INTERNET) == "wlan";
}


JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeHasSomethingToUpload(JNIEnv * env, jclass clazz)
{
  return Editor::Instance().HaveSomethingToUpload();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeUploadChanges(JNIEnv * env, jclass clazz, jstring token, jstring secret)
{
  Editor::Instance().UploadChanges(jni::ToNativeString(env, token),
                                   jni::ToNativeString(env, secret),
                                   {{"version", "TODO android"}}, nullptr);
}

JNIEXPORT jlongArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetStats(JNIEnv * env, jclass clazz)
{
  auto const stats = Editor::Instance().GetStats();
  jlongArray result = env->NewLongArray(3);
  jlong buf[] = {stats.m_edits.size(), stats.m_uploadedCount, stats.m_lastUploadTimestamp};
  env->SetLongArrayRegion(result, 0, 3, buf);
  return result;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeClearLocalEdits(JNIEnv * env, jclass clazz)
{
  Editor::Instance().ClearAllLocalEdits();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeStartEdit(JNIEnv *, jclass)
{
  ::Framework * frm = g_framework->NativeFramework();
  place_page::Info const & info = g_framework->GetPlacePageInfo();
  CHECK(frm->GetEditableMapObject(info.GetID(), g_editableMapObject), ("Invalid feature in the place page."));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeCreateMapObject(JNIEnv *, jclass, jint featureCategory)
{
  ::Framework * frm = g_framework->NativeFramework();
  CHECK(frm->CreateMapObject(frm->GetViewportCenter(), featureCategory, g_editableMapObject),
        ("Couldn't create mapobject, wrong coordinates of missing mwm"));
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetNewFeatureCategories(JNIEnv * env, jclass clazz)
{
  osm::NewFeatureCategories const & printableTypes = g_framework->NativeFramework()->GetEditorCategories();
  int const size = printableTypes.m_allSorted.size();
  auto jCategories = env->NewObjectArray(size, g_featureCategoryClazz, 0);
  for (size_t i = 0; i < size; i++)
  {
    // TODO pass used categories section, too
    jni::TScopedLocalRef jCategory(env, ToJavaFeatureCategory(env, printableTypes.m_allSorted[i]));
    env->SetObjectArrayElement(jCategories, i, jCategory.get());
  }

  return jCategories;
}
} // extern "C"
