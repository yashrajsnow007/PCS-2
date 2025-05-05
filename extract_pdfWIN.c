#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>  // For _mkdir
#include <windows.h>

// Create directory if it doesn't exist
void mkdir_p(const char *path) {
    _mkdir(path);
}

// Run a command and capture output
int run_command(const char *cmd) {
    printf("Running: %s\n", cmd);
    return system(cmd);
}

// Extract PDF text and images using external tools
void extract_pdf_text_and_images(const char *pdf_path, const char *out_txt, const char *img_dir) {
    char cmd[1024];
    
    // Create output directory
    mkdir_p(img_dir);
    
    // 1. Extract text layer with pdftotext (from Poppler)
    sprintf(cmd, "pdftotext -layout \"%s\" \"%s.pdftxt\"", pdf_path, out_txt);
    if (run_command(cmd) != 0) {
        fprintf(stderr, "Error extracting text layer\n");
    }
    
    // 2. Extract images with pdfimages (from Poppler)
    sprintf(cmd, "pdfimages -png \"%s\" \"%s\\page\"", pdf_path, img_dir);
    if (run_command(cmd) != 0) {
        fprintf(stderr, "Error extracting images\n");
    }
    
    // 3. Open the output file
    FILE *f = fopen(out_txt, "w");
    if (!f) {
        fprintf(stderr, "Could not open output file\n");
        return;
    }
    
    // 4. Copy PDF text content first
    FILE *pdftxt = fopen(strcat(strcpy(cmd, out_txt), ".pdftxt"), "r");
    if (pdftxt) {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pdftxt)) {
            fputs(buffer, f);
        }
        fclose(pdftxt);
        // Remove the temporary file
        sprintf(cmd, "%s.pdftxt", out_txt);
        remove(cmd);
    }
    
    // 5. OCR each extracted image with tesseract
    WIN32_FIND_DATA findFileData;
    char searchPath[MAX_PATH];
    sprintf(searchPath, "%s\\*.png", img_dir);
    
    HANDLE hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            char imgPath[MAX_PATH];
            sprintf(imgPath, "%s\\%s", img_dir, findFileData.cFileName);
            
            // Run OCR on this image
            char tempOut[MAX_PATH];
            sprintf(tempOut, "%s\\ocr_temp", img_dir);
            
            // Run tesseract OCR
            sprintf(cmd, "tesseract \"%s\" \"%s\" -l eng", imgPath, tempOut);
            if (run_command(cmd) == 0) {
                // Append OCR results to main output file
                FILE *ocr_out = fopen(strcat(strcpy(cmd, tempOut), ".txt"), "r");
                if (ocr_out) {
                    fprintf(f, "\n\n"); // Separate text chunks
                    
                    char buffer[1024];
                    while (fgets(buffer, sizeof(buffer), ocr_out)) {
                        fputs(buffer, f);
                    }
                    fclose(ocr_out);
                    
                    // Remove temporary OCR file
                    sprintf(cmd, "%s.txt", tempOut);
                    remove(cmd);
                }
            }
            
            // Clean up the image file
            remove(imgPath);
            
        } while (FindNextFile(hFind, &findFileData));
        FindClose(hFind);
    }
    
    fclose(f);
    
    // 6. Try to remove the temporary directory
    _rmdir(img_dir);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input.pdf output.txt image_folder\n", argv[0]);
        return 1;
    }
    extract_pdf_text_and_images(argv[1], argv[2], argv[3]);
    return 0;
}
