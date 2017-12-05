/*
 * Noted by zelbrium
 * Apache License 2.0
 *
 * nc-opensave.c: Load or save a NotedCanvas to/from file.
 */

#include "nc-private.h"
#include "array.h"
#include <arpa/inet.h>
#include <string.h>
#include <math.h>
#include <float.h>

static const uint32_t kMagic1 = 0x819a70ce;

typedef struct
{
    uint16_t npages;
    uint16_t nundo;
    // Followed by npages FilePages
} FileHeader;

typedef struct
{
    uint32_t nstrokes;
    uint16_t pattern; // NCPagePattern
    uint16_t patternDensity;
    NCRect bounds;
    // Followed by nstrokes FileStrokes
} FilePage;

typedef struct
{
    uint32_t npoints;
    NCStrokeStyle style;
    // Followed by npoints x's, then npoints y's
} FileStroke;

//typedef struct
//{
//    uint16_t undoType;
//} FileUndo;


NotedCanvas * load_canvas_v1(FILE *f);

static float ntohf(float val);
static float htonf(float val);


NotedCanvas * noted_canvas_open(const char *path)
{
    uint32_t magic = 0;
    NotedCanvas *canvas = NULL;
    
    FILE *f = fopen(path, "rb");
    
    if(f == NULL)
        return NULL;
    
    if(fread(&magic, sizeof(uint32_t), 1, f) != 1)
    {
        fclose(f);
        return NULL;
    }
    
    // Test file identifier
    switch(magic)
    {
        case kMagic1: canvas = load_canvas_v1(f); break;
    }
    
    canvas->path = strdup(path);
    
    fclose(f);
    return canvas;
}

NotedCanvas * load_canvas_v1(FILE *f)
{
    // Load main canvas object
    FileHeader header;
    if(fread(&header, sizeof(FileHeader), 1, f) != 1)
        return NULL;
    
    header.npages = ntohs(header.npages);
    header.nundo = ntohs(header.nundo);
    
    NotedCanvas *canvas = calloc(1, sizeof(NotedCanvas));
    
    canvas->pages = array_new(sizeof(Page), (FreeNotify)free_page);
    canvas->pages = array_reserve(canvas->pages, header.npages, false);
    
    // Load each page
    for(int i = 0; i < header.npages; ++i)
    {
        FilePage fp;
        if(fread(&fp, sizeof(FilePage), 1, f) != 1)
            goto fail;
        
        // Convert read page data to local endianness
        fp.bounds.x1 = ntohf(fp.bounds.x1);
        fp.bounds.x2 = ntohf(fp.bounds.x2);
        fp.bounds.y1 = ntohf(fp.bounds.y1);
        fp.bounds.y2 = ntohf(fp.bounds.y2);
        fp.nstrokes = ntohl(fp.nstrokes);
        fp.pattern = ntohs(fp.pattern);
        fp.patternDensity = ntohs(fp.patternDensity);

        // Add read page data to canvas
        canvas->pages = array_append(canvas->pages, NULL);
        
        Page *p = &canvas->pages[i];
        
        p->bounds = fp.bounds;
        p->density = fp.patternDensity,
        p->pattern = fp.pattern,
        p->strokes = array_new(sizeof(Stroke), (FreeNotify)free_stroke);
        p->strokes = array_reserve(p->strokes, fp.nstrokes, false);

        // For each stroke in page
        for(int j = 0; j < fp.nstrokes; ++j)
        {
            FileStroke fs;
            if(fread(&fs, sizeof(FileStroke), 1, f) != 1)
                goto fail;
            
            // Convert stroke data to local endianness
            fs.npoints = ntohl(fs.npoints);
            fs.style.thickness = ntohf(fs.style.thickness);

            // Add stroke
            p->strokes = array_append(p->strokes, NULL);
            
            Stroke *s = &p->strokes[j];
            
            s->page = p;
            s->style = fs.style;
            s->x = array_new(sizeof(float), NULL);
            s->y = array_new(sizeof(float), NULL);
            
            if(fs.npoints == 0)
                continue;
            
            // Preallocate space for stroke data
            s->x = array_reserve(s->x, fs.npoints, true);
            s->y = array_reserve(s->y, fs.npoints, true);

            // Read in stroke data
            if(fread(s->x, sizeof(float), fs.npoints, f) != fs.npoints)
                goto fail;
            if(fread(s->y, sizeof(float), fs.npoints, f) != fs.npoints)
                goto fail;
            
            // Calculate stroke's bounding box and maxDistSq
            s->x[0] = ntohf(s->x[0]);
            s->y[0] = ntohf(s->y[0]);
            s->bounds.x1 = s->bounds.x2 = s->x[0];
            s->bounds.y1 = s->bounds.y2 = s->y[0];
            
            for(int k = 1; k < fs.npoints; ++k)
            {
                s->x[k] = ntohf(s->x[k]);
                s->y[k] = ntohf(s->y[k]);
                rect_expand_by_point(&s->bounds, s->x[k], s->y[k]);
                
                float dsq = sq_dist(s->x[k], s->y[k], s->x[k - 1], s->y[k - 1]);
                if(dsq > s->maxDistSq)
                    s->maxDistSq = dsq;
            }
        }
    }
    
    return canvas;
    
fail:
    noted_canvas_destroy(canvas);
    return NULL;
}


bool noted_canvas_save(NotedCanvas *canvas, const char *path)
{
    FILE *f = fopen(path, "wb");
    
    if(f == NULL)
        return NULL;
    
    // Write file identifier
    if(fwrite(&kMagic1, sizeof(uint32_t), 1, f) != 1)
        goto fail;
    
    // Write header
    uint16_t npages = (uint16_t)array_size(canvas->pages);
    
    FileHeader header;
    header.npages = htons(npages);
    header.nundo = 0;
    if(fwrite(&header, sizeof(FileHeader), 1, f) != 1)
        goto fail;
    
    // For each page
    for(uint16_t i = 0; i < npages; ++i)
    {
        uint32_t nstrokes = (uint32_t)array_size(canvas->pages[i].strokes);
        
        // Write page data
        FilePage fp = {
            .bounds = canvas->pages[i].bounds,
            .nstrokes = nstrokes,
            .pattern = canvas->pages[i].pattern,
            .patternDensity = canvas->pages[i].density
        };
        
        // Convert to file endianness
        fp.bounds.x1 = htonf(fp.bounds.x1);
        fp.bounds.x2 = htonf(fp.bounds.x2);
        fp.bounds.y1 = htonf(fp.bounds.y1);
        fp.bounds.y2 = htonf(fp.bounds.y2);
        fp.nstrokes = htonl(fp.nstrokes);
        fp.pattern = htons(fp.pattern);
        fp.patternDensity = htons(fp.patternDensity);
        
        if(fwrite(&fp, sizeof(FilePage), 1, f) != 1)
            goto fail;
        
        // For each stroke in page
        for(uint32_t j = 0; j < nstrokes; ++j)
        {
            // Write stroke data
            uint32_t npoints = (uint32_t)array_size(canvas->pages[i].strokes[j].x);
            
            FileStroke fs = {
                .npoints = npoints,
                .style = canvas->pages[i].strokes[j].style
            };
            
            // Convert to file endianness
            fs.npoints = htonl(fs.npoints);
            fs.style.thickness = htonf(fs.style.thickness);
            
            if(fwrite(&fs, sizeof(FileStroke), 1, f) != 1)
                goto fail;
            
            float p[npoints];
            
            // Convert x's to file endianness and write
            memcpy(p, canvas->pages[i].strokes[j].x, sizeof(float) * npoints);
            for(uint32_t k = 0; k < npoints; ++k)
                p[k] = htonf(p[k]);
            if(fwrite(p, sizeof(float), npoints, f) != npoints)
                goto fail;
            
            // Convert y's and write
            memcpy(p, canvas->pages[i].strokes[j].y, sizeof(float) * npoints);
            for(uint32_t k = 0; k < npoints; ++k)
                p[k] = htonf(p[k]);
            if(fwrite(p, sizeof(float), npoints, f) != npoints)
                goto fail;
        }
    }
    
    fclose(f);
    return true;
    
fail:
    fclose(f);
    return false;
}


/*
 * Convert floats to a "network" byte order and back.
 * TODO: Test on a big-endian machine.
 * Modified from: https://github.com/MalcolmMcLean/ieee754
 */

/*
 * Convert network-order float to host
 */
static float ntohf(float val)
{
    unsigned long mask;
    int sign;
    int exponent;
    int shift;
    int i;
    int significandbits = 23;
    int expbits = 8;
    double fnorm = 0.0;
    double bitval;
    double answer;
    
    union {
        unsigned long lbuff;
        float fbuff;
    } buff;
    
    buff.fbuff = val;
    
    sign = (buff.lbuff & 0x80000000) ? -1 : 1;
    mask = 0x00400000;
    exponent = (buff.lbuff & 0x7F800000) >> 23;
    bitval = 0.5;
    for(i=0;i<significandbits;i++)
    {
        if(buff.lbuff & mask)
            fnorm += bitval;
        bitval /= 2;
        mask >>= 1;
    }
    if(exponent == 0 && fnorm == 0.0)
        return 0.0f;
    shift = exponent - ((1 << (expbits - 1)) - 1); /* exponent = shift + bias */
    
    if(shift == 128 && fnorm != 0.0)
        return (float) sqrt(-1.0);
    if(shift == 128 && fnorm == 0.0)
    {
#ifdef INFINITY
        return sign == 1 ? INFINITY : -INFINITY;
#endif
        return (sign * 1.0f)/0.0f;
    }
    if(shift > -127)
    {
        answer = ldexp(fnorm + 1.0, shift);
        return (float) answer * sign;
    }
    else
    {
        if(fnorm == 0.0)
        {
            return 0.0f;
        }
        shift = -126;
        while (fnorm < 1.0)
        {
            fnorm *= 2;
            shift--;
        }
        answer = ldexp(fnorm, shift);
        return (float) answer * sign;
    }
}

/*
 * Convert host float to network-order
 */
static float htonf(float x)
{
    int shift;
    unsigned long sign, exp, hibits;
    double fnorm, significand;
    int expbits = 8;
    int significandbits = 23;
    
    union {
        unsigned long lbuff;
        float fbuff;
    } buff;
    
    /* zero (can't handle signed zero) */
    if (x == 0)
    {
        buff.lbuff = 0;
        return buff.fbuff;
    }
    /* infinity */
    if (x > FLT_MAX)
    {
        buff.lbuff = 128 + ((1 << (expbits - 1)) - 1);
        buff.lbuff <<= (31 - expbits);
        return buff.fbuff;
    }
    /* -infinity */
    if (x < -FLT_MAX)
    {
        buff.lbuff = 128 + ((1 << (expbits - 1)) - 1);
        buff.lbuff <<= (31 - expbits);
        buff.lbuff |= (1 << 31);
        return buff.fbuff;
    }
    /* NaN - dodgy because many compilers optimise out this test, but
     *there is no portable isnan() */
    if (x != x)
    {
        buff.lbuff = 128 + ((1 << (expbits - 1)) - 1);
        buff.lbuff <<= (31 - expbits);
        buff.lbuff |= 1234;
        return buff.fbuff;
    }
    
    /* get the sign */
    if (x < 0) { sign = 1; fnorm = -x; }
    else { sign = 0; fnorm = x; }
    
    /* get the normalized form of f and track the exponent */
    shift = 0;
    while (fnorm >= 2.0) { fnorm /= 2.0; shift++; }
    while (fnorm < 1.0) { fnorm *= 2.0; shift--; }
    
    /* check for denormalized numbers */
    if (shift < -126)
    {
        while (shift < -126) { fnorm /= 2.0; shift++; }
        shift = -1023;
    }
    /* out of range. Set to infinity */
    else if (shift > 128)
    {
        buff.lbuff = 128 + ((1 << (expbits - 1)) - 1);
        buff.lbuff <<= (31 - expbits);
        buff.lbuff |= (sign << 31);
        return buff.fbuff;
    }
    else
        fnorm = fnorm - 1.0; /* take the significant bit off mantissa */
    
    /* calculate the integer form of the significand */
    /* hold it in a  double for now */
    
    significand = fnorm * ((1LL << significandbits) + 0.5f);
    
    
    /* get the biased exponent */
    exp = shift + ((1 << (expbits - 1)) - 1); /* shift + bias */
    
    hibits = (long)(significand);
    buff.lbuff = (sign << 31) | (exp << (31 - expbits)) | hibits;
    return buff.fbuff;
}

