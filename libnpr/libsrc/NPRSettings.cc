/*****************************************************************************\

NPRSettings.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "NPRSettings.h"

#include <QHash>

NPRSettings NPRSettings::_instance;

const int CURRENT_VERSION = 1;

const QString g_bool_names[NPR_NUM_BOOL_SETTINGS] = 
{
    "enable_lines", /*NPR_ENABLE_LINES*/
    "enable_stylized_lines", /*NPR_ENABLE_STYLIZED_LINES*/
    "enable_polygons", /*NPR_ENABLE_POLYGONS*/
    "enable_transparent_polygons", /*NPR_ENABLE_TRANSPARENT_POLYGONS*/
    "enable_paper_texture", /*NPR_ENABLE_PAPER_TEXTURE*/
    "enable_background_texture", /*NPR_ENABLE_BACKGROUND_TEXTURE*/
    "enable_cutaway", /*NPR_ENABLE_CUTAWAY*/
    "enable_cutaway_lines", /*NPR_ENABLE_CUTAWAY_LINES*/
    "enable_line_artmaps", /*NPR_ENABLE_LINE_ARTMAPS*/
    "enable_color_blur", /*NPR_ENABLE_COLOR_BLUR*/
    "enable_vbos", /*NPR_ENABLE_VBOS*/
    "enable_lighting", /*NPR_ENABLE_LIGHTING*/

    "check_line_visibility", /*NPR_CHECK_LINE_VISIBILITY*/
    "check_line_visibility_at_spine", /*NPR_CHECK_LINE_VISIBILITY_AT_SPINE*/
    "check_line_priority", /*NPR_CHECK_LINE_PRIORITY*/
    "filter_line_visibility", /*NPR_FILTER_LINE_VISIBILITY*/
    "filter_line_priority", /*NPR_FILTER_LINE_PRIORITY*/

    "extract_contours", /*NPR_EXTRACT_CONTOURS*/
    "extract_suggestive_contours", /*NPR_EXTRACT_SUGGESTIVE_CONTOURS*/
    "extract_ridges", /*NPR_EXTRACT_RIDGES*/
    "extract_valleys", /*NPR_EXTRACT_VALLEYS*/
    "extact_isophotes", /*NPR_EXTRACT_ISOPHOTES*/
    "extact_boundaries", /*NPR_EXTRACT_BOUNDARIES*/
    "extract_apparent_ridges", /*NPR_EXTRACT_APPARENT_RIDGES*/
    "extract_profiles", /*NPR_EXTRACT_PROFILES*/
    "classify_boundaries", /*NPR_CLASSIFY_BOUNDARIES*/
    "freeze_lines", /*NPR_FREEZE_LINES*/
    "color_lines_by_id", /*NPR_COLOR_LINES_BY_ID*/

    "view_item_buffer", /*NPR_VIEW_ITEM_BUFFER*/
    "view_priority_buffer", /*NPR_VIEW_PRIORITY_BUFFER*/
    "view_clip_buffer", /*NPR_VIEW_CLIP_BUFFER*/
    "view_tri_strips", /*NPR_VIEW_TRI_STRIPS*/
    "view_cutaway_buffer", /*NPR_VIEW_CUTAWAY_BUFFER*/
    "view_t_param_buffer", /*NPR_VIEW_T_PARAM_BUFFER*/

    "compute_pvs", /*NPR_COMPUTE_PVS*/

    "use_rCSS", /*NPR_USE_RCSS*/
    "use_splatting_gpu", /*NPR_USE_SPLATTING_GPU*/
    "debug_param", /*NPR_DEBUG_PARAM*/
    "enable_gabor_noise", /*NPR_ENABLE_GABOR_NOISE*/
};

const QString g_int_names[NPR_NUM_INT_SETTINGS] = 
{
    "extract_num_isophotes", /*NPR_EXTRACT_NUM_ISOPHOTES*/
    "item_buffer_layers", /*NPR_ITEM_BUFFER_LAYERS*/
    "line_visibility_supersample", /*NPR_LINE_VISIBILITY_SUPERSAMPLE*/

    "focus_mode", /*NPR_FOCUS_MODE*/

    "fitting_iterations", /*NPR_FITTING_ITERATIONS*/
	"fitting_max_level" /*NPR_FITTING_MAX_LEVEL*/
};

const QString g_float_names[NPR_NUM_FLOAT_SETTINGS] = 
{
    "item_buffer_line_width", /*NPR_ITEM_BUFFER_LINE_WIDTH*/
    "segment_atlas_depth_scale", /*NPR_SEGMENT_ATLAS_DEPTH_SCALE*/
    "segment_atlas_kernel_scale_x", /*NPR_SEGMENT_ATLAS_KERNEL_SCALE_X*/
    "segment_atlas_kernel_scale_y", /*NPR_SEGMENT_ATLAS_KERNEL_SCALE_Y*/

    "n_dot_v_bias", /*NPR_N_DOT_V_BIAS*/

    "fitting_threshold", /*NPR_FITTING_THRESOLD*/
    "fitting_max_outliers", /*NPR_FITTING_MAX_OUTLIERS*/
    "fitting_proba_no_outlier" /*NPR_FITTING_PROBA_NO_OUTLIER*/
};

void appendElement(QDomDocument& doc, QDomElement& parent, 
                   const QString& type, const QString& name, const QString& value)
{
    QDomElement element = doc.createElement(type);
    element.setAttribute("name", name);
    element.setAttribute("value", value);
	parent.appendChild(element);
}

bool NPRSettings::load( const QDomElement& element )
{
    int version = element.attribute("version").toInt();
    if (version != CURRENT_VERSION)
    {
        qWarning("NPRSettings::load: settings out of date (version %d, current is %d)\n", version, CURRENT_VERSION);
        return false;
    }

    QHash<QString, int> bool_keys;
    QHash<QString, int> int_keys;
    QHash<QString, int> float_keys;

    for (int i = 0; i < NPR_NUM_BOOL_SETTINGS; i++)
        bool_keys[g_bool_names[i]] = i;
    for (int i = 0; i < NPR_NUM_INT_SETTINGS; i++)
        int_keys[g_int_names[i]] = i;
    for (int i = 0; i < NPR_NUM_FLOAT_SETTINGS; i++)
        float_keys[g_float_names[i]] = i;

    QDomElement entry = element.firstChildElement();
    while (!entry.isNull())
    {
        const QString& name = entry.attribute("name");
        if (entry.tagName() == "bool")
        {
            bool value = entry.attribute("value").toInt();
            int key = bool_keys.value(name, -1);
            if (key >= 0)
                _bools[key] = value;
        }
        else if (entry.tagName() == "int")
        {
            int value = entry.attribute("value").toInt();
            int key = int_keys.value(name, -1);
            if (key >= 0)
                _ints[key] = value;
        }
        else if (entry.tagName() == "float")
        {
            float value = entry.attribute("value").toFloat();
            int key = float_keys.value(name, -1);
            if (key >= 0)
                _floats[key] = value;
        }

        entry = entry.nextSiblingElement();
    }

    return true;
}

bool NPRSettings::save( QDomDocument& doc, QDomElement& element )
{
    element.setAttribute("version", CURRENT_VERSION);

    for (int i = 0; i < NPR_NUM_BOOL_SETTINGS; i++)
        appendElement(doc, element, "bool", g_bool_names[i], QString("%1").arg(_bools[i]));
    for (int i = 0; i < NPR_NUM_INT_SETTINGS; i++)
        appendElement(doc, element, "int", g_int_names[i], QString("%1").arg(_ints[i]));
    for (int i = 0; i < NPR_NUM_FLOAT_SETTINGS; i++)
        appendElement(doc, element, "float", g_float_names[i], QString("%1").arg(_floats[i]));

    return true;
}

void NPRSettings::loadDefaults()
{
    _bools.clear();
    _ints.clear();
    _floats.clear();
    _bools.resize(NPR_NUM_BOOL_SETTINGS);
    _ints.resize(NPR_NUM_INT_SETTINGS);
    _floats.resize(NPR_NUM_FLOAT_SETTINGS);

    _bools[NPR_ENABLE_LINES] = true;
    _bools[NPR_ENABLE_STYLIZED_LINES] = true;
    _bools[NPR_ENABLE_POLYGONS] = true;
    _bools[NPR_ENABLE_TRANSPARENT_POLYGONS] = true;
    _bools[NPR_ENABLE_PAPER_TEXTURE] = true;
    _bools[NPR_ENABLE_BACKGROUND_TEXTURE] = true;
    _bools[NPR_ENABLE_CUTAWAY] = true;
    _bools[NPR_ENABLE_CUTAWAY_LINES] = true;
    _bools[NPR_ENABLE_LINE_ARTMAPS] = false;
    _bools[NPR_ENABLE_COLOR_BLUR] = false;
    _bools[NPR_ENABLE_VBOS] = true;
    _bools[NPR_ENABLE_LIGHTING] = true;

    _bools[NPR_CHECK_LINE_VISIBILITY] = true;
    _bools[NPR_CHECK_LINE_VISIBILITY_AT_SPINE] = true;
    _bools[NPR_CHECK_LINE_PRIORITY] = true;
    _bools[NPR_FILTER_LINE_VISIBILITY] = false;
    _bools[NPR_FILTER_LINE_PRIORITY] = true;

    _bools[NPR_EXTRACT_CONTOURS] = true;
    _bools[NPR_EXTRACT_SUGGESTIVE_CONTOURS] = false;
    _bools[NPR_EXTRACT_RIDGES] = false;
    _bools[NPR_EXTRACT_VALLEYS] = false;
    _bools[NPR_EXTRACT_ISOPHOTES] = false;
    _bools[NPR_EXTRACT_BOUNDARIES] = false;
    _bools[NPR_EXTRACT_APPARENT_RIDGES] = false;
    _bools[NPR_EXTRACT_PROFILES] = true;
    _bools[NPR_CLASSIFY_BOUNDARIES] = true;
    _bools[NPR_FREEZE_LINES] = false;
    _bools[NPR_COLOR_LINES_BY_ID] = false;

    _bools[NPR_VIEW_ITEM_BUFFER] = false;
    _bools[NPR_VIEW_PRIORITY_BUFFER] = false;
    _bools[NPR_VIEW_CLIP_BUFFER] = false;
    _bools[NPR_VIEW_TRI_STRIPS] = false;
    _bools[NPR_VIEW_CUTAWAY_BUFFER] = false;
    _bools[NPR_VIEW_T_PARAM_BUFFER] = false;
    _bools[NPR_ENABLE_GABOR_NOISE] = false;

    _bools[NPR_COMPUTE_PVS] = true;

    _bools[NPR_USE_RCSS] = true;
    _bools[NPR_USE_SPLATTING_GPU] = false;
    _bools[NPR_DEBUG_PARAM] = false;

    _ints[NPR_EXTRACT_NUM_ISOPHOTES] = 10;
    _ints[NPR_ITEM_BUFFER_LAYERS] = 1;
    _ints[NPR_LINE_VISIBILITY_SUPERSAMPLE] = 1;
    _ints[NPR_FOCUS_MODE] = (int)(NPR_FOCUS_NONE);
    _ints[NPR_FITTING_ITERATIONS] = 100;
    _ints[NPR_FITTING_MAX_LEVEL] = 3;
    _ints[NPR_BRUSHPATH_STYLE_MODE] = (int)(NPR_BRUSHPATH_NOFITTING);

    _floats[NPR_ITEM_BUFFER_LINE_WIDTH] = 1;
    _floats[NPR_SEGMENT_ATLAS_DEPTH_SCALE] = 1;
    _floats[NPR_SEGMENT_ATLAS_KERNEL_SCALE_X] = 1;
    _floats[NPR_SEGMENT_ATLAS_KERNEL_SCALE_Y] = 1;

    _floats[NPR_N_DOT_V_BIAS] = -0.1f;
    _floats[NPR_FITTING_THRESHOLD] = 0.1;
    _floats[NPR_FITTING_PROBA_NO_OUTLIER] = 0.95;
    _floats[NPR_FITTING_MAX_OUTLIERS] = 0.80;
}

void NPRSettings::copyPersistent( const NPRSettings& settings )
{
    _bools = settings._bools;
    _ints = settings._ints;
    _floats = settings._floats;
}

