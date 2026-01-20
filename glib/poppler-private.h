#ifndef __POPPLER_PRIVATE_H__
#define __POPPLER_PRIVATE_H__

#include <config.h>

#ifndef __GI_SCANNER__
#    include <PDFDoc.h>
#    include <PSOutputDev.h>
#    include <Link.h>
#    include <Movie.h>
#    include <Rendition.h>
#    include <Form.h>
#    include <Gfx.h>
#    include <FontInfo.h>
#    include <TextOutputDev.h>
#    include <Catalog.h>
#    include <OptionalContent.h>
#    include <CairoOutputDev.h>
#    include <FileSpec.h>
#    include <StructElement.h>
#    include <SignatureInfo.h>
#endif

#define SUPPORTED_ROTATION(r) ((r) == 90 || (r) == 180 || (r) == 270)

struct _PopplerDocument
{
    /*< private >*/
    GObject parent_instance;
    std::unique_ptr<GlobalParamsIniter> initer;
    std::unique_ptr<PDFDoc> doc;

    GList *layers;
    GList *layers_rbgroups;
    CairoOutputDev *output_dev;
};

struct _PopplerPSFile
{
    /*< private >*/
    GObject parent_instance;

    PopplerDocument *document;
    PSOutputDev *out;
    int fd;
    char *filename;
    int first_page;
    int last_page;
    double paper_width;
    double paper_height;
    gboolean duplex;
};

struct _PopplerFontInfo
{
    /*< private >*/
    GObject parent_instance;
    PopplerDocument *document;
    FontInfoScanner *scanner;
};

struct _PopplerPage
{
    /*< private >*/
    GObject parent_instance;
    PopplerDocument *document;
    Page *page;
    int index;
    std::shared_ptr<TextPage> text;
};

struct _PopplerFormField
{
    /*< private >*/
    GObject parent_instance;
    PopplerDocument *document;
    FormWidget *widget;
    PopplerAction *action;
    PopplerAction *field_modified_action;
    PopplerAction *format_field_action;
    PopplerAction *validate_field_action;
    PopplerAction *calculate_field_action;
};

struct _PopplerAnnot
{
    GObject parent_instance;
    std::shared_ptr<Annot> annot;
};

struct _PopplerPath
{
    PopplerPoint *points;
    gsize n_points;
};

struct Layer
{
    /*< private >*/
    GList *kids;
    gchar *label;
    OptionalContentGroup *oc;
};

struct _PopplerLayer
{
    /*< private >*/
    GObject parent_instance;
    PopplerDocument *document;
    Layer *layer;
    GList *rbgroup;
    gchar *title;
};

struct _PopplerStructureElement
{
    /*< private >*/
    GObject parent_instance;
    PopplerDocument *document;
    const StructElement *elem;
};

/*
 * PopplerRectangleExtended:
 *
 * The real type behind the public PopplerRectangle.
 * Must be ABI compatible to it!
 */
struct PopplerRectangleExtended
{
    /*< private >*/
    double x1;
    double y1;
    double x2;
    double y2;
    bool match_continued; /* Described in poppler_rectangle_find_get_match_continued() */
    bool ignored_hyphen; /* Described in poppler_rectangle_find_get_ignored_hyphen() */
};

PopplerRectangle *poppler_rectangle_new_from_pdf_rectangle(const PDFRectangle *rect);

GList *_poppler_document_get_layers(PopplerDocument *document);
GList *_poppler_document_get_layer_rbgroup(PopplerDocument *document, Layer *layer);
PopplerPage *_poppler_page_new(PopplerDocument *document, Page *page, int index);
void _unrotate_rect_for_annot_and_page(Page *page, Annot *annot, double *x1, double *y1, double *x2, double *y2);
void _page_unrotate_xy(Page *page, double *x, double *y);
void _page_rotate_xy(Page *page, double *x, double *y);
AnnotQuadrilaterals *_page_new_quads_unrotated(Page *page, AnnotQuadrilaterals *quads);
AnnotQuadrilaterals *new_quads_from_offset_cropbox(const PDFRectangle *crop_box, AnnotQuadrilaterals *quads, gboolean add);
PopplerAction *_poppler_action_new(PopplerDocument *document, const LinkAction *link, const gchar *title);
PopplerLayer *_poppler_layer_new(PopplerDocument *document, Layer *layer, GList *rbgroup);
PopplerDest *_poppler_dest_new_goto(PopplerDocument *document, LinkDest *link_dest);
PopplerFormField *_poppler_form_field_new(PopplerDocument *document, FormWidget *field);
PopplerAttachment *_poppler_attachment_new(FileSpec *file);
PopplerMovie *_poppler_movie_new(const Movie *movie);
PopplerMedia *_poppler_media_new(const MediaRendition *media);
PopplerAnnot *_poppler_annot_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_text_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_free_text_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_text_markup_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_file_attachment_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_movie_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_screen_new(PopplerDocument *doc, const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_line_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_circle_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_square_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_stamp_new(const std::shared_ptr<Annot> &annot);
PopplerAnnot *_poppler_annot_ink_new(const std::shared_ptr<Annot> &annot);

const PDFRectangle *_poppler_annot_get_cropbox(PopplerAnnot *poppler_annot);

char *_poppler_goo_string_to_utf8(const GooString *s);
gboolean _poppler_convert_pdf_date_to_gtime(const GooString *date, time_t *gdate);
GDateTime *_poppler_convert_pdf_date_to_date_time(const GooString *date);
std::unique_ptr<GooString> _poppler_convert_date_time_to_pdf_date(GDateTime *datetime);
AnnotStampImageHelper *_poppler_convert_cairo_image_to_stamp_image_helper(const cairo_surface_t *image);
PopplerColor *_poppler_convert_annot_color_to_poppler_color(const AnnotColor *color);
std::unique_ptr<AnnotColor> _poppler_convert_poppler_color_to_annot_color(const PopplerColor *poppler_color);

void _poppler_error_cb(ErrorCategory category, Goffset pos, const char *message);

#endif
