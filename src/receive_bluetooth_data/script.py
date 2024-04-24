from flask import Flask, request, jsonify
app = Flask(__name__)

@app.route('/', methods=["GET"])
def hello():
    return "Hello world!"

@app.route('/post-run', methods=['POST'])
def add_message():
    content = request.json
    print(content)
    return {"content": "received"}

if __name__ == '__main__':
    app.run(host= '0.0.0.0', port=42069, debug=False)