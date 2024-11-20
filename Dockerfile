# שלב 1: הורדת דימוי בסיס שמכיל את gcc
FROM gcc:latest

# שלב 2: הגדרת תיקיית העבודה בקונטיינר
WORKDIR /usr/src/app

# שלב 3: העתקת קובץ ה-C לתוך הקונטיינר
COPY submarine.c .

# שלב 4: קימפול קובץ ה-C
RUN gcc -o submarine submarine.c

# שלב 5: הגדרת פקודת ברירת המחדל להרצת התוכנית
CMD ["./submarine"]