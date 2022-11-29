from flask import Flask, render_template

@app.route('/')
def index():
    return render_template('index.html', sw_state_list = sw_state_list)

if __name__ == "__main__":
   app.run(host="0.0.0.0", port = "8080")