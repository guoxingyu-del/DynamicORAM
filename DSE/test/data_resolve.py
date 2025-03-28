import pickle
import os

def load_pkl(file_path):
    if not os.path.isfile(file_path):
        raise FileNotFoundError(f"{file_path} not exist")
    
    try:
        with open(file_path, "rb") as f:
            data = pickle.load(f)

            return data
    except Exception as e:
        print(f"load failure: {str(e)}")
        raise

data = load_pkl("lucene_doc.pkl")
print(data)