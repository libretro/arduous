class Speaker {
   public:
    Speaker() = default;
    Speaker(const Speaker&) = delete;
    Speaker(Speaker&&) = delete;
    Speaker& operator=(const Speaker&) = delete;
    Speaker& operator=(Speaker&&) = delete;
    ~Speaker() = default;

    void nextFrame();

   private:
}
